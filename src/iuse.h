#ifndef _IUSE_H_
#define _IUSE_H_

class item;
class player;

// iuse methods returning a bool indicating whether to consume a charge of the item being used.
class iuse
{
 public:
  int none             (player *p, item *it, bool t);

// FOOD AND DRUGS (ADMINISTRATION)
  int sewage           (player *p, item *it, bool t);
  int honeycomb        (player *p, item *it, bool t);
  int royal_jelly      (player *p, item *it, bool t);
  int bandage          (player *p, item *it, bool t);
  int firstaid         (player *p, item *it, bool t);
  int completefirstaid (player *p, item *it, bool t);
  int disinfectant     (player *p, item *it, bool t);
  int caff             (player *p, item *it, bool t);
  int atomic_caff      (player *p, item *it, bool t);
  int alcohol          (player *p, item *it, bool t);
  int alcohol_weak     (player *p, item *it, bool t);
  int pkill            (player *p, item *it, bool t);
  int xanax            (player *p, item *it, bool t);
  int cig              (player *p, item *it, bool t);
  int antibiotic       (player *p, item *it, bool t);
  int fungicide        (player *p, item *it, bool t);
  int weed             (player *p, item *it, bool t);
  int coke             (player *p, item *it, bool t);
  int crack            (player *p, item *it, bool t);
  int grack            (player *p, item *it, bool t);
  int meth             (player *p, item *it, bool t);
  int vitamins         (player *p, item *it, bool t);
  int vaccine          (player *p, item *it, bool t);
  int poison           (player *p, item *it, bool t);
  int hallu            (player *p, item *it, bool t);
  int thorazine        (player *p, item *it, bool t);
  int prozac           (player *p, item *it, bool t);
  int sleep            (player *p, item *it, bool t);
  int iodine           (player *p, item *it, bool t);
  int flumed           (player *p, item *it, bool t);
  int flusleep         (player *p, item *it, bool t);
  int inhaler          (player *p, item *it, bool t);
  int blech            (player *p, item *it, bool t);
  int chew            (player *p, item *it, bool t);
  int mutagen          (player *p, item *it, bool t);
  int mut_iv          (player *p, item *it, bool t);
  int purifier         (player *p, item *it, bool t);
  int purify_iv          (player *p, item *it, bool t);
  int marloss          (player *p, item *it, bool t);
  int dogfood          (player *p, item *it, bool t);
  int catfood          (player *p, item *it, bool t);

// TOOLS
  int lighter          (player *p, item *it, bool t);
  int primitive_fire   (player *p, item *it, bool t);
  int sew              (player *p, item *it, bool t);
  int extra_battery    (player *p, item *it, bool t);
  int rechargeable_battery(player *p, item *it, bool t);
  int cut_up           (player *p, item *it, item *cut, bool t);
  int scissors         (player *p, item *it, bool t);
  int extinguisher     (player *p, item *it, bool t);
  int hammer           (player *p, item *it, bool t);
  int light_off        (player *p, item *it, bool t);
  int light_on         (player *p, item *it, bool t);
  int gasoline_lantern_off (player *p, item *it, bool t);
  int gasoline_lantern_on  (player *p, item *it, bool t);
  int lightstrip       (player *p, item *it, bool t);
  int lightstrip_active(player *p, item *it, bool t);
  int glowstick        (player *p, item *it, bool t);
  int glowstick_active (player *p, item *it, bool t);
  int solder_weld      (player *p, item *it, bool t);
  int water_purifier   (player *p, item *it, bool t);
  int two_way_radio    (player *p, item *it, bool t);
  int directional_antenna (player *p, item *it, bool t);
  int radio_off        (player *p, item *it, bool t);
  int radio_on         (player *p, item *it, bool t);
  int horn_bicycle     (player *p, item *it, bool t);
  int noise_emitter_off(player *p, item *it, bool t);
  int noise_emitter_on (player *p, item *it, bool t);
  int roadmap          (player *p, item *it, bool t);
  int survivormap      (player *p, item *it, bool t);
  int militarymap      (player *p, item *it, bool t);
  int restaurantmap    (player *p, item *it, bool t);
  int touristmap       (player *p, item *it, bool t);
  int picklock         (player *p, item *it, bool t);
  int crowbar          (player *p, item *it, bool t);
  int makemound        (player *p, item *it, bool t);
  int dig              (player *p, item *it, bool t);
  int siphon           (player *p, item *it, bool t);
  int chainsaw_off     (player *p, item *it, bool t);
  int chainsaw_on      (player *p, item *it, bool t);
  int cs_lajatang_off  (player *p, item *it, bool t);
  int cs_lajatang_on   (player *p, item *it, bool t);
  int carver_off       (player *p, item *it, bool t);
  int carver_on        (player *p, item *it, bool t);
  int trimmer_off      (player *p, item *it, bool t);
  int trimmer_on       (player *p, item *it, bool t);
  int circsaw_off      (player *p, item *it, bool t);
  int circsaw_on       (player *p, item *it, bool t);
  int combatsaw_off    (player *p, item *it, bool t);
  int combatsaw_on     (player *p, item *it, bool t);
  int shishkebab_off   (player *p, item *it, bool t);
  int shishkebab_on    (player *p, item *it, bool t);
  int firemachete_off  (player *p, item *it, bool t);
  int firemachete_on   (player *p, item *it, bool t);
  int broadfire_off    (player *p, item *it, bool t);
  int broadfire_on     (player *p, item *it, bool t);
  int firekatana_off   (player *p, item *it, bool t);
  int firekatana_on    (player *p, item *it, bool t);
  int zweifire_off     (player *p, item *it, bool t);
  int zweifire_on      (player *p, item *it, bool t);
  int jackhammer       (player *p, item *it, bool t);
  int jacqueshammer    (player *p, item *it, bool t);
  int pickaxe          (player *p, item *it, bool t);
  int set_trap         (player *p, item *it, bool t);
  int geiger           (player *p, item *it, bool t);
  int teleport         (player *p, item *it, bool t);
  int can_goo          (player *p, item *it, bool t);
  int pipebomb         (player *p, item *it, bool t);
  int pipebomb_act     (player *p, item *it, bool t);
  int grenade          (player *p, item *it, bool t);
  int grenade_act      (player *p, item *it, bool t);
  int granade          (player *p, item *it, bool t);
  int granade_act      (player *p, item *it, bool t);
  int flashbang        (player *p, item *it, bool t);
  int flashbang_act    (player *p, item *it, bool t);
  int c4               (player *p, item *it, bool t);
  int c4armed          (player *p, item *it, bool t);
  int EMPbomb          (player *p, item *it, bool t);
  int EMPbomb_act      (player *p, item *it, bool t);
  int scrambler        (player *p, item *it, bool t);
  int scrambler_act    (player *p, item *it, bool t);
  int gasbomb          (player *p, item *it, bool t);
  int gasbomb_act      (player *p, item *it, bool t);
  int smokebomb        (player *p, item *it, bool t);
  int smokebomb_act    (player *p, item *it, bool t);
  int acidbomb         (player *p, item *it, bool t);
  int arrow_flamable   (player *p, item *it, bool t);
  int acidbomb_act     (player *p, item *it, bool t);
  int molotov          (player *p, item *it, bool t);
  int molotov_lit      (player *p, item *it, bool t);
  int matchbomb        (player *p, item *it, bool t);
  int matchbomb_act    (player *p, item *it, bool t);
  int dynamite         (player *p, item *it, bool t);
  int dynamite_act     (player *p, item *it, bool t);
  int firecracker_pack (player *p, item *it, bool t);
  int firecracker_pack_act (player *p, item *it, bool t);
  int firecracker      (player *p, item *it, bool t);
  int firecracker_act  (player *p, item *it, bool t);
  int mininuke         (player *p, item *it, bool t);
  int mininuke_act     (player *p, item *it, bool t);
  int pheromone        (player *p, item *it, bool t);
  int portal           (player *p, item *it, bool t);
  int manhack          (player *p, item *it, bool t);
  int turret           (player *p, item *it, bool t);
  int turret_laser     (player *p, item *it, bool t);
  int UPS_off          (player *p, item *it, bool t);
  int UPS_on           (player *p, item *it, bool t);
  int adv_UPS_off      (player *p, item *it, bool t);
  int adv_UPS_on       (player *p, item *it, bool t);
  int tazer            (player *p, item *it, bool t);
  int tazer2           (player *p, item *it, bool t);
  int shocktonfa_off   (player *p, item *it, bool t);
  int shocktonfa_on    (player *p, item *it, bool t);
  int mp3              (player *p, item *it, bool t);
  int mp3_on           (player *p, item *it, bool t);
  int portable_game    (player *p, item *it, bool t);
  int vortex           (player *p, item *it, bool t);
  int dog_whistle      (player *p, item *it, bool t);
  int vacutainer       (player *p, item *it, bool t);
  int knife            (player *p, item *it, bool t);
  static int cut_log_into_planks(player *p, item *it);
  int lumber           (player *p, item *it, bool t);
  int hacksaw          (player *p, item *it, bool t);
  int tent             (player *p, item *it, bool t);
  int shelter          (player *p, item *it, bool t);
  int torch            (player *p, item *it, bool t);
  int torch_lit        (player *p, item *it, bool t);
  int handflare        (player *p, item *it, bool t);
  int handflare_lit    (player *p, item *it, bool t);
  int battletorch      (player *p, item *it, bool t);
  int battletorch_lit  (player *p, item *it, bool t);
  int candle           (player *p, item *it, bool t);
  int candle_lit       (player *p, item *it, bool t);
  int bullet_puller    (player *p, item *it, bool t);
  int boltcutters      (player *p, item *it, bool t);
  int mop              (player *p, item *it, bool t);
  int spray_can        (player *p, item *it, bool t);
  int rag              (player *p, item *it, bool t);
  int pda              (player *p, item *it, bool t);
  int pda_flashlight   (player *p, item *it, bool t);
  int LAW              (player *p, item *it, bool t);
  int heatpack         (player *p, item *it, bool t);
  int hotplate         (player *p, item *it, bool t);
  int dejar            (player *p, item *it, bool t);
  int rad_badge        (player *p, item *it, bool t);
  int boots            (player *p, item *it, bool t);
  int towel            (player *p, item *it, bool t);
  int unfold_bicycle   (player *p, item *it, bool t);
  int airhorn          (player *p, item *it, bool t);
  int adrenaline_injector (player *p, item *it, bool t);
  int jet_injector     (player *p, item *it, bool t);
  int contacts (player *p, item *it, bool t);
  int talking_doll     (player *p, item *it, bool t);
  int bell             (player *p, item *it, bool t);
  int oxygen_bottle          (player *p, item *it, bool t);
// MACGUFFINS
  int mcg_note         (player *p, item *it, bool t);
// ARTIFACTS
// This function is used when an artifact is activated
// It examines the item's artifact-specific properties
// See artifact.h for a list
  int artifact         (player *p, item *it, bool t);
};


typedef int (iuse::*use_function_pointer)(player*,item*,bool);

enum use_function_t {
    USE_FUNCTION_CPP,
    USE_FUNCTION_LUA
};

struct use_function {
    use_function_t function_type;

    union {
        use_function_pointer cpp_function;
        int lua_function;
    };

    use_function() : function_type(USE_FUNCTION_CPP) {};

    use_function(use_function_pointer f)
        : function_type(USE_FUNCTION_CPP), cpp_function(f)
    { };

    use_function(int f)
        : function_type(USE_FUNCTION_LUA), lua_function(f)
    { };

    int call(player*,item*,bool);

    void operator=(use_function_pointer f) {
        cpp_function = f;
    }

    bool operator==(use_function_pointer f) const {
        return (function_type == USE_FUNCTION_CPP) && (f == cpp_function);
    }

    bool operator!=(use_function_pointer f) const {
        return !(this->operator==(f));
    }
};


#endif
