#ifndef _IUSE_H_
#define _IUSE_H_

class item;
class player;

// iuse methods returning a bool indicating whether to consume a charge of the item being used.
class iuse
{
public:
    int none                (player*, item*, bool);

// FOOD AND DRUGS (ADMINISTRATION)
    int sewage              (player*, item*, bool);
    int honeycomb           (player*, item*, bool);
    int royal_jelly         (player*, item*, bool);
    int bandage             (player*, item*, bool);
    int firstaid            (player*, item*, bool);
    int completefirstaid    (player*, item*, bool);
    int disinfectant        (player*, item*, bool);
    int caff                (player*, item*, bool);
    int atomic_caff         (player*, item*, bool);
    int alcohol             (player*, item*, bool);
    int alcohol_weak        (player*, item*, bool);
    int pkill               (player*, item*, bool);
    int xanax               (player*, item*, bool);
    int cig                 (player*, item*, bool);
    int antibiotic          (player*, item*, bool);
    int eyedrops            (player*, item*, bool);
    int fungicide           (player*, item*, bool);
    int antifungal          (player*, item*, bool);
    int weed                (player*, item*, bool);
    int coke                (player*, item*, bool);
    int crack               (player*, item*, bool);
    int grack               (player*, item*, bool);
    int meth                (player*, item*, bool);
    int vitamins            (player*, item*, bool);
    int vaccine             (player*, item*, bool);
    int poison              (player*, item*, bool);
    int hallu               (player*, item*, bool);
    int fun_hallu           (player*, item*, bool);
    int thorazine           (player*, item*, bool);
    int prozac              (player*, item*, bool);
    int sleep               (player*, item*, bool);
    int iodine              (player*, item*, bool);
    int flumed              (player*, item*, bool);
    int flusleep            (player*, item*, bool);
    int inhaler             (player*, item*, bool);
    int blech               (player*, item*, bool);
    int chew                (player*, item*, bool);
    int mutagen             (player*, item*, bool);
    int mut_iv              (player*, item*, bool);
    int purifier            (player*, item*, bool);
    int purify_iv           (player*, item*, bool);
    int marloss             (player*, item*, bool);
    int dogfood             (player*, item*, bool);
    int catfood             (player*, item*, bool);

// TOOLS
    int lighter             (player *, item *, bool);
    int primitive_fire      (player *, item *, bool);
    int sew                 (player *, item *, bool);
    int extra_battery       (player *, item *, bool);
    int rechargeable_battery(player *, item *, bool);
    int cut_up              (player *, item *, item *, bool);
    int scissors            (player *, item *, bool);
    int extinguisher        (player *, item *, bool);
    int hammer              (player *, item *, bool);
    int light_off           (player *, item *, bool);
    int light_on            (player *, item *, bool);
    int gasoline_lantern_off(player *, item *, bool);
    int gasoline_lantern_on (player *, item *, bool);
    int oil_lamp_off        (player *, item *, bool);
    int oil_lamp_on         (player *, item *, bool);
    int lightstrip          (player *, item *, bool);
    int lightstrip_active   (player *, item *, bool);
    int glowstick           (player *, item *, bool);
    int glowstick_active    (player *, item *, bool);
    int solder_weld         (player *, item *, bool);
    int water_purifier      (player *, item *, bool);
    int two_way_radio       (player *, item *, bool);
    int directional_antenna (player *, item *, bool);
    int radio_off           (player *, item *, bool);
    int radio_on            (player *, item *, bool);
    int horn_bicycle        (player *, item *, bool);
    int noise_emitter_off   (player *, item *, bool);
    int noise_emitter_on    (player *, item *, bool);
    int roadmap             (player *, item *, bool);
    int survivormap         (player *, item *, bool);
    int militarymap         (player *, item *, bool);
    int restaurantmap       (player *, item *, bool);
    int touristmap          (player *, item *, bool);
    int ma_manual           (player *, item *, bool);
    int picklock            (player *, item *, bool);
    int crowbar             (player *, item *, bool);
    int makemound           (player *, item *, bool);
    int dig                 (player *, item *, bool);
    int siphon              (player *, item *, bool);
    int chainsaw_off        (player *, item *, bool);
    int chainsaw_on         (player *, item *, bool);
    int cs_lajatang_off     (player *, item *, bool);
    int cs_lajatang_on      (player *, item *, bool);
    int carver_off          (player *, item *, bool);
    int carver_on           (player *, item *, bool);
    int trimmer_off         (player *, item *, bool);
    int trimmer_on          (player *, item *, bool);
    int circsaw_off         (player *, item *, bool);
    int circsaw_on          (player *, item *, bool);
    int combatsaw_off       (player *, item *, bool);
    int combatsaw_on        (player *, item *, bool);
    int shishkebab_off      (player *, item *, bool);
    int shishkebab_on       (player *, item *, bool);
    int firemachete_off     (player *, item *, bool);
    int firemachete_on      (player *, item *, bool);
    int broadfire_off       (player *, item *, bool);
    int broadfire_on        (player *, item *, bool);
    int firekatana_off      (player *, item *, bool);
    int firekatana_on       (player *, item *, bool);
    int zweifire_off        (player *, item *, bool);
    int zweifire_on         (player *, item *, bool);
    int jackhammer          (player *, item *, bool);
    int jacqueshammer       (player *, item *, bool);
    int pickaxe             (player *, item *, bool);
    int set_trap            (player *, item *, bool);
    int geiger              (player *, item *, bool);
    int teleport            (player *, item *, bool);
    int can_goo             (player *, item *, bool);
    int throwable_extinguisher_act(player *, item *, bool);
    int pipebomb            (player *, item *, bool);
    int pipebomb_act        (player *, item *, bool);
    int grenade             (player *, item *, bool);
    int grenade_act         (player *, item *, bool);
    int granade             (player *, item *, bool);
    int granade_act         (player *, item *, bool);
    int flashbang           (player *, item *, bool);
    int flashbang_act       (player *, item *, bool);
    int c4                  (player *, item *, bool);
    int c4armed             (player *, item *, bool);
    int EMPbomb             (player *, item *, bool);
    int EMPbomb_act         (player *, item *, bool);
    int scrambler           (player *, item *, bool);
    int scrambler_act       (player *, item *, bool);
    int gasbomb             (player *, item *, bool);
    int gasbomb_act         (player *, item *, bool);
    int smokebomb           (player *, item *, bool);
    int smokebomb_act       (player *, item *, bool);
    int acidbomb            (player *, item *, bool);
    int arrow_flamable      (player *, item *, bool);
    int acidbomb_act        (player *, item *, bool);
    int molotov             (player *, item *, bool);
    int molotov_lit         (player *, item *, bool);
    int matchbomb           (player *, item *, bool);
    int matchbomb_act       (player *, item *, bool);
    int dynamite            (player *, item *, bool);
    int dynamite_act        (player *, item *, bool);
    int firecracker_pack    (player *, item *, bool);
    int firecracker_pack_act(player *, item *, bool);
    int firecracker         (player *, item *, bool);
    int firecracker_act     (player *, item *, bool);
    int mininuke            (player *, item *, bool);
    int mininuke_act        (player *, item *, bool);
    int pheromone           (player *, item *, bool);
    int portal              (player *, item *, bool);
    int manhack             (player *, item *, bool);
    int turret              (player *, item *, bool);
    int turret_laser        (player *, item *, bool);
    int UPS_off             (player *, item *, bool);
    int UPS_on              (player *, item *, bool);
    int adv_UPS_off         (player *, item *, bool);
    int adv_UPS_on          (player *, item *, bool);
    int tazer               (player *, item *, bool);
    int tazer2              (player *, item *, bool);
    int shocktonfa_off      (player *, item *, bool);
    int shocktonfa_on       (player *, item *, bool);
    int mp3                 (player *, item *, bool);
    int mp3_on              (player *, item *, bool);
    int portable_game       (player *, item *, bool);
    int vortex              (player *, item *, bool);
    int dog_whistle         (player *, item *, bool);
    int vacutainer          (player *, item *, bool);
    int knife               (player *, item *, bool);
    static int cut_log_into_planks(player *p, item *it);
    int lumber              (player *, item *, bool);
    int hacksaw             (player *, item *, bool);
    int tent                (player *, item *, bool);
    int shelter             (player *, item *, bool);
    int torch               (player *, item *, bool);
    int torch_lit           (player *, item *, bool);
    int handflare           (player *, item *, bool);
    int handflare_lit       (player *, item *, bool);
    int battletorch         (player *, item *, bool);
    int battletorch_lit     (player *, item *, bool);
    int candle              (player *, item *, bool);
    int candle_lit          (player *, item *, bool);
    int bullet_puller       (player *, item *, bool);
    int boltcutters         (player *, item *, bool);
    int mop                 (player *, item *, bool);
    int spray_can           (player *, item *, bool);
    int rag                 (player *, item *, bool);
    int pda                 (player *, item *, bool);
    int pda_flashlight      (player *, item *, bool);
    int LAW                 (player *, item *, bool);
    int heatpack            (player *, item *, bool);
    int hotplate            (player *, item *, bool);
    int dejar               (player *, item *, bool);
    int rad_badge           (player *, item *, bool);
    int boots               (player *, item *, bool);
    int towel               (player *, item *, bool);
    int unfold_bicycle      (player *, item *, bool);
    int airhorn             (player *, item *, bool);
    int adrenaline_injector (player *, item *, bool);
    int jet_injector        (player *, item *, bool);
    int contacts            (player *, item *, bool);
    int talking_doll        (player *, item *, bool);
    int bell                (player *, item *, bool);
    int oxygen_bottle       (player *, item *, bool);
    int atomic_battery      (player *, item *, bool);
    int fishing_rod_basic   (player *, item *, bool);
// MACGUFFINS
    int mcg_note            (player *, item *, bool);

// ARTIFACTS
    /* This function is used when an artifact is activated.
       It examines the item's artifact-specific properties.
       See artifact.h for a list.                        */
    int artifact            (player *, item *, bool);
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
