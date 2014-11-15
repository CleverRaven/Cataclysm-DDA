#ifndef IUSE_H
#define IUSE_H

#include "monstergenerator.h"
#include <map>
#include <string>
#include <vector>

class item;
class player;
class JsonObject;

// iuse methods returning a bool indicating whether to consume a charge of the item being used.
class iuse
{
public:
// FOOD AND DRUGS (ADMINISTRATION)
    int raw_meat            (player*, item*, bool, tripoint);
    int raw_fat             (player*, item*, bool, tripoint);
    int raw_bone            (player*, item*, bool, tripoint);
    int raw_fish            (player*, item*, bool, tripoint);
    int raw_wildveg         (player*, item*, bool, tripoint);
    int sewage              (player*, item*, bool, tripoint);
    int honeycomb           (player*, item*, bool, tripoint);
    int royal_jelly         (player*, item*, bool, tripoint);
    int bandage             (player*, item*, bool, tripoint);
    int firstaid            (player*, item*, bool, tripoint);
    int completefirstaid    (player*, item*, bool, tripoint);
    int disinfectant        (player*, item*, bool, tripoint);
    int caff                (player*, item*, bool, tripoint);
    int atomic_caff         (player*, item*, bool, tripoint);
    int alcohol             (player*, item*, bool, tripoint);
    int alcohol_weak        (player*, item*, bool, tripoint);
    int alcohol_strong      (player*, item*, bool, tripoint);
    int xanax               (player*, item*, bool, tripoint);
    int smoking             (player*, item*, bool, tripoint);
    int smoking_pipe        (player*, item*, bool, tripoint);
    int ecig                (player*, item*, bool, tripoint);
    int antibiotic          (player*, item*, bool, tripoint);
    int eyedrops            (player*, item*, bool, tripoint);
    int fungicide           (player*, item*, bool, tripoint);
    int antifungal          (player*, item*, bool, tripoint);
    int antiparasitic       (player*, item*, bool, tripoint);
    int anticonvulsant      (player*, item*, bool, tripoint);
    int weed_brownie        (player*, item*, bool, tripoint);
    int coke                (player*, item*, bool, tripoint);
    int grack               (player*, item*, bool, tripoint);
    int meth                (player*, item*, bool, tripoint);
    int vitamins            (player*, item*, bool, tripoint);
    int vaccine             (player*, item*, bool, tripoint);
    int poison              (player*, item*, bool, tripoint);
    int fun_hallu           (player*, item*, bool, tripoint);
    int thorazine           (player*, item*, bool, tripoint);
    int prozac              (player*, item*, bool, tripoint);
    int sleep               (player*, item*, bool, tripoint);
    int iodine              (player*, item*, bool, tripoint);
    int datura              (player*, item*, bool, tripoint);
    int flumed              (player*, item*, bool, tripoint);
    int flusleep            (player*, item*, bool, tripoint);
    int inhaler             (player*, item*, bool, tripoint);
    int blech               (player*, item*, bool, tripoint);
    int plantblech          (player*, item*, bool, tripoint);
    int chew                (player*, item*, bool, tripoint);
    int mutagen             (player*, item*, bool, tripoint);
    int mut_iv              (player*, item*, bool, tripoint);
    int purifier            (player*, item*, bool, tripoint);
    int purify_iv           (player*, item*, bool, tripoint);
    int marloss             (player*, item*, bool, tripoint);
    int marloss_seed        (player*, item*, bool, tripoint);
    int marloss_gel         (player*, item*, bool, tripoint);
    int mycus               (player*, item*, bool, tripoint);
    int dogfood             (player*, item*, bool, tripoint);
    int catfood             (player*, item*, bool, tripoint);

// TOOLS
    int firestarter         (player *, item *, bool, tripoint);
    int resolve_firestarter_use(player *p, item *, tripoint);
    int calculate_time_for_lens_fire (player *p, float light_level);
    int sew                 (player *, item *, bool, tripoint);
    int extra_battery       (player *, item *, bool, tripoint);
    int rechargeable_battery(player *, item *, bool, tripoint);
    int scissors            (player *, item *, bool, tripoint);
    int extinguisher        (player *, item *, bool, tripoint);
    int hammer              (player *, item *, bool, tripoint);
    int solder_weld         (player *, item *, bool, tripoint);
    int water_purifier      (player *, item *, bool, tripoint);
    int two_way_radio       (player *, item *, bool, tripoint);
    int directional_antenna (player *, item *, bool, tripoint);
    int radio_off           (player *, item *, bool, tripoint);
    int radio_on            (player *, item *, bool, tripoint);
    int horn_bicycle        (player *, item *, bool, tripoint);
    int noise_emitter_off   (player *, item *, bool, tripoint);
    int noise_emitter_on    (player *, item *, bool, tripoint);
    int roadmap             (player *, item *, bool, tripoint);
    int survivormap         (player *, item *, bool, tripoint);
    int militarymap         (player *, item *, bool, tripoint);
    int restaurantmap       (player *, item *, bool, tripoint);
    int touristmap          (player *, item *, bool, tripoint);
    int ma_manual           (player *, item *, bool, tripoint);
    int picklock            (player *, item *, bool, tripoint);
    int crowbar             (player *, item *, bool, tripoint);
    int makemound           (player *, item *, bool, tripoint);
    int dig                 (player *, item *, bool, tripoint);
    int siphon              (player *, item *, bool, tripoint);
    int chainsaw_off        (player *, item *, bool, tripoint);
    int chainsaw_on         (player *, item *, bool, tripoint);
    int cs_lajatang_off     (player *, item *, bool, tripoint);
    int cs_lajatang_on      (player *, item *, bool, tripoint);
    int carver_off          (player *, item *, bool, tripoint);
    int carver_on           (player *, item *, bool, tripoint);
    int trimmer_off         (player *, item *, bool, tripoint);
    int trimmer_on          (player *, item *, bool, tripoint);
    int circsaw_on          (player *, item *, bool, tripoint);
    int combatsaw_off       (player *, item *, bool, tripoint);
    int combatsaw_on        (player *, item *, bool, tripoint);
    int shishkebab_off      (player *, item *, bool, tripoint);
    int shishkebab_on       (player *, item *, bool, tripoint);
    int firemachete_off     (player *, item *, bool, tripoint);
    int firemachete_on      (player *, item *, bool, tripoint);
    int broadfire_off       (player *, item *, bool, tripoint);
    int broadfire_on        (player *, item *, bool, tripoint);
    int firekatana_off      (player *, item *, bool, tripoint);
    int firekatana_on       (player *, item *, bool, tripoint);
    int zweifire_off        (player *, item *, bool, tripoint);
    int zweifire_on         (player *, item *, bool, tripoint);
    int jackhammer          (player *, item *, bool, tripoint);
    int jacqueshammer       (player *, item *, bool, tripoint);
    int pickaxe             (player *, item *, bool, tripoint);
    int set_trap            (player *, item *, bool, tripoint);
    int geiger              (player *, item *, bool, tripoint);
    int teleport            (player *, item *, bool, tripoint);
    int can_goo             (player *, item *, bool, tripoint);
    int throwable_extinguisher_act(player *, item *, bool, tripoint);
    int pipebomb_act        (player *, item *, bool, tripoint);
    int granade             (player *, item *, bool, tripoint);
    int granade_act         (player *, item *, bool, tripoint);
    int c4                  (player *, item *, bool, tripoint);
    int arrow_flamable      (player *, item *, bool, tripoint);
    int acidbomb_act        (player *, item *, bool, tripoint);
    int grenade_inc_act     (player *, item *, bool, tripoint);
    int molotov             (player *, item *, bool, tripoint);
    int molotov_lit         (player *, item *, bool, tripoint);
    int firecracker_pack    (player *, item *, bool, tripoint);
    int firecracker_pack_act(player *, item *, bool, tripoint);
    int firecracker         (player *, item *, bool, tripoint);
    int firecracker_act     (player *, item *, bool, tripoint);
    int mininuke            (player *, item *, bool, tripoint);
    int pheromone           (player *, item *, bool, tripoint);
    int portal              (player *, item *, bool, tripoint);
    int UPS_off             (player *, item *, bool, tripoint);
    int UPS_on              (player *, item *, bool, tripoint);
    int adv_UPS_off         (player *, item *, bool, tripoint);
    int adv_UPS_on          (player *, item *, bool, tripoint);
    int tazer               (player *, item *, bool, tripoint);
    int tazer2              (player *, item *, bool, tripoint);
    int shocktonfa_off      (player *, item *, bool, tripoint);
    int shocktonfa_on       (player *, item *, bool, tripoint);
    int mp3                 (player *, item *, bool, tripoint);
    int mp3_on              (player *, item *, bool, tripoint);
    int portable_game       (player *, item *, bool, tripoint);
    int vibe                (player *, item *, bool, tripoint);
    int vortex              (player *, item *, bool, tripoint);
    int dog_whistle         (player *, item *, bool, tripoint);
    int vacutainer          (player *, item *, bool, tripoint);
    int knife               (player *, item *, bool, tripoint);
    static int cut_log_into_planks(player *p, item *it);
    int lumber              (player *, item *, bool, tripoint);
    int oxytorch            (player *, item *, bool, tripoint);
    int hacksaw             (player *, item *, bool, tripoint);
    int portable_structure  (player *, item *, bool, tripoint);
    int tent                (player *, item *, bool, tripoint);
    int large_tent          (player *, item *, bool, tripoint);
    int shelter             (player *, item *, bool, tripoint);
    int torch_lit           (player *, item *, bool, tripoint);
    int battletorch_lit     (player *, item *, bool, tripoint);
    int bullet_puller       (player *, item *, bool, tripoint);
    int boltcutters         (player *, item *, bool, tripoint);
    int mop                 (player *, item *, bool, tripoint);
    int spray_can           (player *, item *, bool, tripoint);
    int rag                 (player *, item *, bool, tripoint);
    int LAW                 (player *, item *, bool, tripoint);
    int heatpack            (player *, item *, bool, tripoint);
    int hotplate            (player *, item *, bool, tripoint);
    int flask_yeast         (player *, item *, bool, tripoint);
    int tanning_hide        (player *, item *, bool, tripoint);
    int quiver              (player *, item *, bool, tripoint);
    int boots               (player *, item *, bool, tripoint);
    int sheath_sword        (player *, item *, bool, tripoint);
    int sheath_knife        (player *, item *, bool, tripoint);
    int holster_pistol      (player *, item *, bool, tripoint);
    int holster_ankle       (player *, item *, bool, tripoint);
    int towel               (player *, item *, bool, tripoint);
    int unfold_generic      (player *, item *, bool, tripoint);
    int airhorn             (player *, item *, bool, tripoint);
    int adrenaline_injector (player *, item *, bool, tripoint);
    int jet_injector        (player *, item *, bool, tripoint);
    int contacts            (player *, item *, bool, tripoint);
    int talking_doll        (player *, item *, bool, tripoint);
    int bell                (player *, item *, bool, tripoint);
    int seed                (player *, item *, bool, tripoint);
    int oxygen_bottle       (player *, item *, bool, tripoint);
    int atomic_battery      (player *, item *, bool, tripoint);
    int ups_battery         (player *, item *, bool, tripoint);
    int fishing_rod         (player *, item *, bool, tripoint);
    int fish_trap           (player *, item *, bool, tripoint);
    int gun_repair          (player *, item *, bool, tripoint);
    int misc_repair         (player *, item *, bool, tripoint);
    int rm13armor_off       (player *, item *, bool, tripoint);
    int rm13armor_on        (player *, item *, bool, tripoint);
    int unpack_item         (player *, item *, bool, tripoint);
    int pack_item           (player *, item *, bool, tripoint);
    int radglove            (player *, item *, bool, tripoint);
    int robotcontrol        (player *, item *, bool, tripoint);
    int einktabletpc        (player *, item *, bool, tripoint);
    int camera              (player *, item *, bool, tripoint);
    int ehandcuffs          (player *, item *, bool, tripoint);
    int cable_attach        (player *, item *, bool, tripoint);
    int pocket_meteorolgist (player *, item *, bool, tripoint);
    int survivor_belt       (player *, item *, bool, tripoint);

// MACGUFFINS
    int mcg_note            (player *, item *, bool, tripoint);

    int radiocar(player *, item *, bool, tripoint);
    int radiocaron(player *, item *, bool, tripoint);
    int radiocontrol(player *, item *, bool, tripoint);

    int multicooker(player *, item *, bool, tripoint);

// ARTIFACTS
    /* This function is used when an artifact is activated.
       It examines the item's artifact-specific properties.
       See artifact.h for a list.                        */
    int artifact            (player *, item *, bool, point);

    // Helper for listening to music, might deserve a better home, but not sure where.
    static void play_music( player *p, point source, int volume );

    static void reset_bullet_pulling();
    static void load_bullet_pulling(JsonObject &jo);
protected:
    typedef std::pair<std::string, int> result_t;
    typedef std::vector<result_t> result_list_t;
    typedef std::map<std::string, result_list_t> bullet_pulling_t;
    static bullet_pulling_t bullet_pulling_recipes;
};


typedef int (iuse::*use_function_pointer)(player*,item*,bool, point);

class iuse_actor {
protected:
    iuse_actor() { }
public:
    virtual ~iuse_actor() { }
    virtual long use(player*, item*, bool, point) const = 0;
    virtual iuse_actor *clone() const = 0;
};

struct use_function {
protected:
    enum use_function_t {
        USE_FUNCTION_NONE,
        USE_FUNCTION_CPP,
        USE_FUNCTION_ACTOR_PTR,
        USE_FUNCTION_LUA
    };

    use_function_t function_type;

    union {
        use_function_pointer cpp_function;
        int lua_function;
        iuse_actor *actor_ptr;
    };

public:
    use_function()
        : function_type(USE_FUNCTION_NONE)
    { }

    use_function(use_function_pointer f)
        : function_type(USE_FUNCTION_CPP), cpp_function(f)
    { }

    use_function(int f)
        : function_type(USE_FUNCTION_LUA), lua_function(f)
    { }

    use_function(iuse_actor *f)
        : function_type(USE_FUNCTION_ACTOR_PTR), actor_ptr(f)
    { }

    use_function(const use_function &other);

    ~use_function();

    int call(player*,item*,bool,point) const;

    void operator=(use_function_pointer f);
    void operator=(iuse_actor *f);
    void operator=(const use_function &other);

    bool operator==(use_function f) const {
        return function_type == USE_FUNCTION_CPP && f.function_type == USE_FUNCTION_CPP &&
        f.cpp_function == cpp_function;
    }

    bool operator==(use_function_pointer f) const {
        return (function_type == USE_FUNCTION_CPP) && (f == cpp_function);
    }

    bool operator!=(use_function_pointer f) const {
        return !(this->operator==(f));
    }
};

#endif
