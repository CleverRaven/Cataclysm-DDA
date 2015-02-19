#ifndef IUSE_H
#define IUSE_H

#include <map>
#include <string>
#include <vector>
#include "enums.h"

class item;
class player;
class JsonObject;
class MonsterGenerator;

// iuse methods returning a bool indicating whether to consume a charge of the item being used.
namespace iuse {
// FOOD AND DRUGS (ADMINISTRATION)
    int raw_meat            (player*, item*, bool, point);
    int raw_fat             (player*, item*, bool, point);
    int raw_bone            (player*, item*, bool, point);
    int raw_fish            (player*, item*, bool, point);
    int raw_wildveg         (player*, item*, bool, point);
    int sewage              (player*, item*, bool, point);
    int honeycomb           (player*, item*, bool, point);
    int royal_jelly         (player*, item*, bool, point);
    int bandage             (player*, item*, bool, point);
    int firstaid            (player*, item*, bool, point);
    int completefirstaid    (player*, item*, bool, point);
    int disinfectant        (player*, item*, bool, point);
    int caff                (player*, item*, bool, point);
    int atomic_caff         (player*, item*, bool, point);
    int alcohol             (player*, item*, bool, point);
    int alcohol_weak        (player*, item*, bool, point);
    int alcohol_strong      (player*, item*, bool, point);
    int xanax               (player*, item*, bool, point);
    int smoking             (player*, item*, bool, point);
    int smoking_pipe        (player*, item*, bool, point);
    int ecig                (player*, item*, bool, point);
    int antibiotic          (player*, item*, bool, point);
    int eyedrops            (player*, item*, bool, point);
    int fungicide           (player*, item*, bool, point);
    int antifungal          (player*, item*, bool, point);
    int antiparasitic       (player*, item*, bool, point);
    int anticonvulsant      (player*, item*, bool, point);
    int weed_brownie        (player*, item*, bool, point);
    int coke                (player*, item*, bool, point);
    int grack               (player*, item*, bool, point);
    int meth                (player*, item*, bool, point);
    int vitamins            (player*, item*, bool, point);
    int vaccine             (player*, item*, bool, point);
    int flu_vaccine         (player*, item*, bool, point);
    int poison              (player*, item*, bool, point);
    int fun_hallu           (player*, item*, bool, point);
    int meditate            (player*, item*, bool, point);
    int thorazine           (player*, item*, bool, point);
    int prozac              (player*, item*, bool, point);
    int sleep               (player*, item*, bool, point);
    int iodine              (player*, item*, bool, point);
    int datura              (player*, item*, bool, point);
    int flumed              (player*, item*, bool, point);
    int flusleep            (player*, item*, bool, point);
    int inhaler             (player*, item*, bool, point);
    int blech               (player*, item*, bool, point);
    int plantblech          (player*, item*, bool, point);
    int chew                (player*, item*, bool, point);
    int mutagen             (player*, item*, bool, point);
    int mut_iv              (player*, item*, bool, point);
    int purifier            (player*, item*, bool, point);
    int purify_iv           (player*, item*, bool, point);
    int marloss             (player*, item*, bool, point);
    int marloss_seed        (player*, item*, bool, point);
    int marloss_gel         (player*, item*, bool, point);
    int mycus               (player*, item*, bool, point);
    int dogfood             (player*, item*, bool, point);
    int catfood             (player*, item*, bool, point);
// TOOLS
    int sew                 (player *, item *, bool, point);
    int sew_advanced        (player *, item *, bool, point);
    int extra_battery       (player *, item *, bool, point);
    int rechargeable_battery(player *, item *, bool, point);
    int scissors            (player *, item *, bool, point);
    int extinguisher        (player *, item *, bool, point);
    int hammer              (player *, item *, bool, point);
    int solder_weld         (player *, item *, bool, point);
    int water_purifier      (player *, item *, bool, point);
    int two_way_radio       (player *, item *, bool, point);
    int directional_antenna (player *, item *, bool, point);
    int radio_off           (player *, item *, bool, point);
    int radio_on            (player *, item *, bool, point);
    int horn_bicycle        (player *, item *, bool, point);
    int noise_emitter_off   (player *, item *, bool, point);
    int noise_emitter_on    (player *, item *, bool, point);
    int ma_manual           (player *, item *, bool, point);
    int crowbar             (player *, item *, bool, point);
    int makemound           (player *, item *, bool, point);
    int dig                 (player *, item *, bool, point);
    int siphon              (player *, item *, bool, point);
    int chainsaw_off        (player *, item *, bool, point);
    int chainsaw_on         (player *, item *, bool, point);
    int elec_chainsaw_off   (player *, item *, bool, point);
    int elec_chainsaw_on    (player *, item *, bool, point);
    int cs_lajatang_off     (player *, item *, bool, point);
    int cs_lajatang_on      (player *, item *, bool, point);
    int carver_off          (player *, item *, bool, point);
    int carver_on           (player *, item *, bool, point);
    int trimmer_off         (player *, item *, bool, point);
    int trimmer_on          (player *, item *, bool, point);
    int circsaw_on          (player *, item *, bool, point);
    int combatsaw_off       (player *, item *, bool, point);
    int combatsaw_on        (player *, item *, bool, point);
    int shishkebab_off      (player *, item *, bool, point);
    int shishkebab_on       (player *, item *, bool, point);
    int firemachete_off     (player *, item *, bool, point);
    int firemachete_on      (player *, item *, bool, point);
    int broadfire_off       (player *, item *, bool, point);
    int broadfire_on        (player *, item *, bool, point);
    int firekatana_off      (player *, item *, bool, point);
    int firekatana_on       (player *, item *, bool, point);
    int zweifire_off        (player *, item *, bool, point);
    int zweifire_on         (player *, item *, bool, point);
    int jackhammer          (player *, item *, bool, point);
    int jacqueshammer       (player *, item *, bool, point);
    int pickaxe             (player *, item *, bool, point);
    int set_trap            (player *, item *, bool, point);
    int geiger              (player *, item *, bool, point);
    int teleport            (player *, item *, bool, point);
    int can_goo             (player *, item *, bool, point);
    int throwable_extinguisher_act(player *, item *, bool, point);
    int pipebomb_act        (player *, item *, bool, point);
    int granade             (player *, item *, bool, point);
    int granade_act         (player *, item *, bool, point);
    int c4                  (player *, item *, bool, point);
    int arrow_flamable      (player *, item *, bool, point);
    int acidbomb_act        (player *, item *, bool, point);
    int grenade_inc_act     (player *, item *, bool, point);
    int molotov             (player *, item *, bool, point);
    int molotov_lit         (player *, item *, bool, point);
    int firecracker_pack    (player *, item *, bool, point);
    int firecracker_pack_act(player *, item *, bool, point);
    int firecracker         (player *, item *, bool, point);
    int firecracker_act     (player *, item *, bool, point);
    int mininuke            (player *, item *, bool, point);
    int pheromone           (player *, item *, bool, point);
    int portal              (player *, item *, bool, point);
    int UPS_off             (player *, item *, bool, point);
    int UPS_on              (player *, item *, bool, point);
    int adv_UPS_off         (player *, item *, bool, point);
    int adv_UPS_on          (player *, item *, bool, point);
    int tazer               (player *, item *, bool, point);
    int tazer2              (player *, item *, bool, point);
    int shocktonfa_off      (player *, item *, bool, point);
    int shocktonfa_on       (player *, item *, bool, point);
    int mp3                 (player *, item *, bool, point);
    int mp3_on              (player *, item *, bool, point);
    int portable_game       (player *, item *, bool, point);
    int vibe                (player *, item *, bool, point);
    int vortex              (player *, item *, bool, point);
    int dog_whistle         (player *, item *, bool, point);
    int vacutainer          (player *, item *, bool, point);
    int knife               (player *, item *, bool, point);
    int lumber              (player *, item *, bool, point);
    int oxytorch            (player *, item *, bool, point);
    int hacksaw             (player *, item *, bool, point);
    int portable_structure  (player *, item *, bool, point);
    int tent                (player *, item *, bool, point);
    int large_tent          (player *, item *, bool, point);
    int shelter             (player *, item *, bool, point);
    int torch_lit           (player *, item *, bool, point);
    int battletorch_lit     (player *, item *, bool, point);
    int bullet_puller       (player *, item *, bool, point);
    int boltcutters         (player *, item *, bool, point);
    int mop                 (player *, item *, bool, point);
    int spray_can           (player *, item *, bool, point);
    int rag                 (player *, item *, bool, point);
    int LAW                 (player *, item *, bool, point);
    int heatpack            (player *, item *, bool, point);
    int hotplate            (player *, item *, bool, point);
    int quiver              (player *, item *, bool, point);
    int boots               (player *, item *, bool, point);
    int sheath_sword        (player *, item *, bool, point);
    int sheath_knife        (player *, item *, bool, point);
    int holster_gun         (player *, item *, bool, point);
    int holster_ankle       (player *, item *, bool, point);
    int towel               (player *, item *, bool, point);
    int unfold_generic      (player *, item *, bool, point);
    int airhorn             (player *, item *, bool, point);
    int adrenaline_injector (player *, item *, bool, point);
    int jet_injector        (player *, item *, bool, point);
    int contacts            (player *, item *, bool, point);
    int talking_doll        (player *, item *, bool, point);
    int bell                (player *, item *, bool, point);
    int seed                (player *, item *, bool, point);
    int oxygen_bottle       (player *, item *, bool, point);
    int atomic_battery      (player *, item *, bool, point);
    int ups_battery         (player *, item *, bool, point);
    int remove_all_mods     (player *, item *, bool, point);
    int fishing_rod         (player *, item *, bool, point);
    int fish_trap           (player *, item *, bool, point);
    int gun_repair          (player *, item *, bool, point);
    int misc_repair         (player *, item *, bool, point);
    int rm13armor_off       (player *, item *, bool, point);
    int rm13armor_on        (player *, item *, bool, point);
    int unpack_item         (player *, item *, bool, point);
    int pack_item           (player *, item *, bool, point);
    int radglove            (player *, item *, bool, point);
    int robotcontrol        (player *, item *, bool, point);
    int einktabletpc        (player *, item *, bool, point);
    int camera              (player *, item *, bool, point);
    int ehandcuffs          (player *, item *, bool, point);
    int cable_attach        (player *, item *, bool, point);
    int weather_tool        (player *, item *, bool, point);
    int survivor_belt       (player *, item *, bool, point);
// MACGUFFINS
    int mcg_note            (player *, item *, bool, point);
    int radiocar            (player *, item *, bool, point);
    int radiocaron          (player *, item *, bool, point);
    int radiocontrol        (player *, item *, bool, point);
    int multicooker         (player *, item *, bool, point);
    int remoteveh           (player *, item *, bool, point);
// ARTIFACTS
    /* This function is used when an artifact is activated.
       It examines the item's artifact-specific properties.
       See artifact.h for a list.                        */
    int artifact            (player *, item *, bool, point);

    bool valid_to_cut_up(const item *it);
    int cut_up              (player *p, item *it, item *cut, bool);
    int cut_log_into_planks (player *p, item *it);

    // Helper for listening to music, might deserve a better home, but not sure where.
    void play_music( player *p, point source, int volume );

    void reset_bullet_pulling();
    void load_bullet_pulling(JsonObject &jo);
};

using use_function_pointer = int (*)(player*, item*, bool, point);

class iuse_actor {
protected:
    iuse_actor() = default;
public:
    std::string type;
    virtual ~iuse_actor() = default;
    virtual long use(player*, item*, bool, point) const = 0;
    virtual bool can_use( const player*, const item*, bool, const point& ) const { return true; }
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

    iuse_actor *get_actor_ptr() const
    {
        if( function_type != USE_FUNCTION_ACTOR_PTR ) {
            return nullptr;
        }
        return actor_ptr;
    }

    void operator=(use_function_pointer f);
    void operator=(iuse_actor *f);
    void operator=(const use_function &other);

    bool operator==(use_function f) const {
        if( function_type != f.function_type ) {
            return false;
        }

        switch( function_type ) {
            case USE_FUNCTION_NONE:
                return true;
            case USE_FUNCTION_CPP:
                return f.cpp_function == cpp_function;
            case USE_FUNCTION_ACTOR_PTR:
                return f.actor_ptr->type == actor_ptr->type;
            case USE_FUNCTION_LUA:
                return f.lua_function == lua_function;
            default:
                return false;
        }
    }

    bool operator==(use_function_pointer f) const {
        return (function_type == USE_FUNCTION_CPP) && (f == cpp_function);
    }

    bool operator!=(use_function_pointer f) const {
        return !(this->operator==(f));
    }
};

#endif
