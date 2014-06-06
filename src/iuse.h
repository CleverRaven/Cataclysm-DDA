#ifndef _IUSE_H_
#define _IUSE_H_

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
    int raw_meat            (player*, item*, bool);
    int raw_fat             (player*, item*, bool);
    int raw_bone            (player*, item*, bool);
    int raw_fish            (player*, item*, bool);
    int raw_wildveg         (player*, item*, bool);
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
    int alcohol_strong      (player*, item*, bool);
    int xanax               (player*, item*, bool);
    int smoking             (player*, item*, bool);
    int smoking_pipe        (player*, item*, bool);
    int ecig                (player*, item*, bool);
    int antibiotic          (player*, item*, bool);
    int eyedrops            (player*, item*, bool);
    int fungicide           (player*, item*, bool);
    int antifungal          (player*, item*, bool);
    int antiparasitic       (player*, item*, bool);
    int anticonvulsant      (player*, item*, bool);
    int weed_brownie        (player*, item*, bool);
    int coke                (player*, item*, bool);
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
    int pipebomb_act        (player *, item *, bool);
    int granade             (player *, item *, bool);
    int granade_act         (player *, item *, bool);
    int c4                  (player *, item *, bool);
    int arrow_flamable      (player *, item *, bool);
    int acidbomb_act        (player *, item *, bool);
    int molotov             (player *, item *, bool);
    int molotov_lit         (player *, item *, bool);
    int firecracker_pack    (player *, item *, bool);
    int firecracker_pack_act(player *, item *, bool);
    int firecracker         (player *, item *, bool);
    int firecracker_act     (player *, item *, bool);
    int mininuke            (player *, item *, bool);
    int pheromone           (player *, item *, bool);
    int portal              (player *, item *, bool);
    int manhack             (player *, item *, bool);
    int turret              (player *, item *, bool);
    int turret_laser        (player *, item *, bool);
    int turret_rifle        (player *, item *, bool);
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
    int vibe                (player *, item *, bool);
    int vortex              (player *, item *, bool);
    int dog_whistle         (player *, item *, bool);
    int vacutainer          (player *, item *, bool);
    int knife               (player *, item *, bool);
    static int cut_log_into_planks(player *p, item *it);
    int lumber              (player *, item *, bool);
    int hacksaw             (player *, item *, bool);
    int tent                (player *, item *, bool);
    int shelter             (player *, item *, bool);
    int torch_lit           (player *, item *, bool);
    int battletorch_lit     (player *, item *, bool);
    int bullet_puller       (player *, item *, bool);
    int boltcutters         (player *, item *, bool);
    int mop                 (player *, item *, bool);
    int spray_can           (player *, item *, bool);
    int rag                 (player *, item *, bool);
    int LAW                 (player *, item *, bool);
    int heatpack            (player *, item *, bool);
    int hotplate            (player *, item *, bool);
    int flask_yeast         (player *, item *, bool);
    int quiver              (player *, item *, bool);
    int boots               (player *, item *, bool);
    int sheath_sword        (player *, item *, bool);
    int sheath_knife        (player *, item *, bool);
    int holster_pistol      (player *, item *, bool);
    int holster_ankle       (player *, item *, bool);
    int towel               (player *, item *, bool);
    int unfold_generic      (player *, item *, bool);
    int airhorn             (player *, item *, bool);
    int adrenaline_injector (player *, item *, bool);
    int jet_injector        (player *, item *, bool);
    int contacts            (player *, item *, bool);
    int talking_doll        (player *, item *, bool);
    int bell                (player *, item *, bool);
    int seed                (player *, item *, bool);
    int oxygen_bottle       (player *, item *, bool);
    int atomic_battery      (player *, item *, bool);
    int ups_battery         (player *, item *, bool);
    int fishing_rod_basic   (player *, item *, bool);
    int gun_repair          (player *, item *, bool);
    int misc_repair         (player *, item *, bool);
    int rm13armor_off       (player *, item *, bool);
    int rm13armor_on        (player *, item *, bool);
    int unpack_item         (player *, item *, bool);
    int pack_item           (player *, item *, bool);
    int radglove            (player *, item *, bool);
    int robotcontrol        (player *, item *, bool);
// MACGUFFINS
    int mcg_note            (player *, item *, bool);

// ARTIFACTS
    /* This function is used when an artifact is activated.
       It examines the item's artifact-specific properties.
       See artifact.h for a list.                        */
    int artifact            (player *, item *, bool);

    static void reset_bullet_pulling();
    static void load_bullet_pulling(JsonObject &jo);
protected:
    typedef std::pair<std::string, int> result_t;
    typedef std::vector<result_t> result_list_t;
    typedef std::map<std::string, result_list_t> bullet_pulling_t;
    static bullet_pulling_t bullet_pulling_recipes;
};


typedef int (iuse::*use_function_pointer)(player*,item*,bool);

class iuse_actor {
protected:
    iuse_actor() { }
public:
    virtual ~iuse_actor() { }
    virtual long use(player*, item*, bool) const = 0;
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

    int call(player*,item*,bool) const;

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
