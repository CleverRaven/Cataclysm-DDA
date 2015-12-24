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
class iuse
{
public:
// FOOD AND DRUGS (ADMINISTRATION)
    int raw_meat            ( player*, item*, bool, const tripoint& );
    int raw_fat             ( player*, item*, bool, const tripoint& );
    int raw_bone            ( player*, item*, bool, const tripoint& );
    int raw_fish            ( player*, item*, bool, const tripoint& );
    int raw_wildveg         ( player*, item*, bool, const tripoint& );
    int sewage              ( player*, item*, bool, const tripoint& );
    int honeycomb           ( player*, item*, bool, const tripoint& );
    int royal_jelly         ( player*, item*, bool, const tripoint& );
    int bandage             ( player*, item*, bool, const tripoint& );
    int firstaid            ( player*, item*, bool, const tripoint& );
    int completefirstaid    ( player*, item*, bool, const tripoint& );
    int disinfectant        ( player*, item*, bool, const tripoint& );
    int caff                ( player*, item*, bool, const tripoint& );
    int atomic_caff         ( player*, item*, bool, const tripoint& );
    int alcohol_weak        ( player*, item*, bool, const tripoint& );
    int alcohol_medium      ( player*, item*, bool, const tripoint& );
    int alcohol_strong      ( player*, item*, bool, const tripoint& );
    int xanax               ( player*, item*, bool, const tripoint& );
    int smoking             ( player*, item*, bool, const tripoint& );
    int smoking_pipe        ( player*, item*, bool, const tripoint& );
    int ecig                ( player*, item*, bool, const tripoint& );
    int antibiotic          ( player*, item*, bool, const tripoint& );
    int eyedrops            ( player*, item*, bool, const tripoint& );
    int fungicide           ( player*, item*, bool, const tripoint& );
    int antifungal          ( player*, item*, bool, const tripoint& );
    int antiparasitic       ( player*, item*, bool, const tripoint& );
    int anticonvulsant      ( player*, item*, bool, const tripoint& );
    int weed_brownie        ( player*, item*, bool, const tripoint& );
    int coke                ( player*, item*, bool, const tripoint& );
    int grack               ( player*, item*, bool, const tripoint& );
    int meth                ( player*, item*, bool, const tripoint& );
    int vitamins            ( player*, item*, bool, const tripoint& );
    int vaccine             ( player*, item*, bool, const tripoint& );
    int flu_vaccine         ( player*, item*, bool, const tripoint& );
    int poison              ( player*, item*, bool, const tripoint& );
    int fun_hallu           ( player*, item*, bool, const tripoint& );
    int meditate            ( player*, item*, bool, const tripoint& );
    int thorazine           ( player*, item*, bool, const tripoint& );
    int prozac              ( player*, item*, bool, const tripoint& );
    int sleep               ( player*, item*, bool, const tripoint& );
    int iodine              ( player*, item*, bool, const tripoint& );
    int datura              ( player*, item*, bool, const tripoint& );
    int flumed              ( player*, item*, bool, const tripoint& );
    int flusleep            ( player*, item*, bool, const tripoint& );
    int inhaler             ( player*, item*, bool, const tripoint& );
    int blech               ( player*, item*, bool, const tripoint& );
    int plantblech          ( player*, item*, bool, const tripoint& );
    int chew                ( player*, item*, bool, const tripoint& );
    int mutagen             ( player*, item*, bool, const tripoint& );
    int mut_iv              ( player*, item*, bool, const tripoint& );
    int purifier            ( player*, item*, bool, const tripoint& );
    int purify_iv           ( player*, item*, bool, const tripoint& );
    int marloss             ( player*, item*, bool, const tripoint& );
    int marloss_seed        ( player*, item*, bool, const tripoint& );
    int marloss_gel         ( player*, item*, bool, const tripoint& );
    int mycus               ( player*, item*, bool, const tripoint& );
    int dogfood             ( player*, item*, bool, const tripoint& );
    int catfood             ( player*, item*, bool, const tripoint& );

// TOOLS
    int sew_advanced        ( player*, item*, bool, const tripoint& );
    int extra_battery       ( player*, item*, bool, const tripoint& );
    int double_reactor      ( player*, item*, bool, const tripoint& );
    int rechargeable_battery( player*, item*, bool, const tripoint& );
    int scissors            ( player*, item*, bool, const tripoint& );
    int extinguisher        ( player*, item*, bool, const tripoint& );
    int hammer              ( player*, item*, bool, const tripoint& );
    int water_purifier      ( player*, item*, bool, const tripoint& );
    int two_way_radio       ( player*, item*, bool, const tripoint& );
    int directional_antenna ( player*, item*, bool, const tripoint& );
    int radio_off           ( player*, item*, bool, const tripoint& );
    int radio_on            ( player*, item*, bool, const tripoint& );
    int noise_emitter_off   ( player*, item*, bool, const tripoint& );
    int noise_emitter_on    ( player*, item*, bool, const tripoint& );
    int ma_manual           ( player*, item*, bool, const tripoint& );
    int crowbar             ( player*, item*, bool, const tripoint& );
    int makemound           ( player*, item*, bool, const tripoint& );
    int dig                 ( player*, item*, bool, const tripoint& );
    int siphon              ( player*, item*, bool, const tripoint& );
    int chainsaw_off        ( player*, item*, bool, const tripoint& );
    int chainsaw_on         ( player*, item*, bool, const tripoint& );
    int elec_chainsaw_off   ( player*, item*, bool, const tripoint& );
    int elec_chainsaw_on    ( player*, item*, bool, const tripoint& );
    int cs_lajatang_off     ( player*, item*, bool, const tripoint& );
    int cs_lajatang_on      ( player*, item*, bool, const tripoint& );
    int carver_off          ( player*, item*, bool, const tripoint& );
    int carver_on           ( player*, item*, bool, const tripoint& );
    int trimmer_off         ( player*, item*, bool, const tripoint& );
    int trimmer_on          ( player*, item*, bool, const tripoint& );
    int circsaw_on          ( player*, item*, bool, const tripoint& );
    int combatsaw_off       ( player*, item*, bool, const tripoint& );
    int combatsaw_on        ( player*, item*, bool, const tripoint& );
    int jackhammer          ( player*, item*, bool, const tripoint& );
    int jacqueshammer       ( player*, item*, bool, const tripoint& );
    int pickaxe             ( player*, item*, bool, const tripoint& );
    int set_trap            ( player*, item*, bool, const tripoint& );
    int geiger              ( player*, item*, bool, const tripoint& );
    int teleport            ( player*, item*, bool, const tripoint& );
    int can_goo             ( player*, item*, bool, const tripoint& );
    int throwable_extinguisher_act( player*, item*, bool, const tripoint& );
    int capture_monster_act ( player*, item*, bool, const tripoint& );
    int pipebomb_act        ( player*, item*, bool, const tripoint& );
    int granade             ( player*, item*, bool, const tripoint& );
    int granade_act         ( player*, item*, bool, const tripoint& );
    int c4                  ( player*, item*, bool, const tripoint& );
    int arrow_flamable      ( player*, item*, bool, const tripoint& );
    int acidbomb_act        ( player*, item*, bool, const tripoint& );
    int grenade_inc_act     ( player*, item*, bool, const tripoint& );
    int molotov             ( player*, item*, bool, const tripoint& );
    int molotov_lit         ( player*, item*, bool, const tripoint& );
    int firecracker_pack    ( player*, item*, bool, const tripoint& );
    int firecracker_pack_act( player*, item*, bool, const tripoint& );
    int firecracker         ( player*, item*, bool, const tripoint& );
    int firecracker_act     ( player*, item*, bool, const tripoint& );
    int mininuke            ( player*, item*, bool, const tripoint& );
    int pheromone           ( player*, item*, bool, const tripoint& );
    int portal              ( player*, item*, bool, const tripoint& );
    int UPS_off             ( player*, item*, bool, const tripoint& );
    int UPS_on              ( player*, item*, bool, const tripoint& );
    int adv_UPS_off         ( player*, item*, bool, const tripoint& );
    int adv_UPS_on          ( player*, item*, bool, const tripoint& );
    int tazer               ( player*, item*, bool, const tripoint& );
    int tazer2              ( player*, item*, bool, const tripoint& );
    int shocktonfa_off      ( player*, item*, bool, const tripoint& );
    int shocktonfa_on       ( player*, item*, bool, const tripoint& );
    int mp3                 ( player*, item*, bool, const tripoint& );
    int mp3_on              ( player*, item*, bool, const tripoint& );
    int portable_game       ( player*, item*, bool, const tripoint& );
    int vibe                ( player*, item*, bool, const tripoint& );
    int vortex              ( player*, item*, bool, const tripoint& );
    int dog_whistle         ( player*, item*, bool, const tripoint& );
    int vacutainer          ( player*, item*, bool, const tripoint& );
    static void cut_log_into_planks(player *);
    int lumber              ( player*, item*, bool, const tripoint& );
    int oxytorch            ( player*, item*, bool, const tripoint& );
    int hacksaw             ( player*, item*, bool, const tripoint& );
    int portable_structure  ( player*, item*, bool, const tripoint& );
    int tent                ( player*, item*, bool, const tripoint& );
    int large_tent          ( player*, item*, bool, const tripoint& );
    int shelter             ( player*, item*, bool, const tripoint& );
    int torch_lit           ( player*, item*, bool, const tripoint& );
    int battletorch_lit     ( player*, item*, bool, const tripoint& );
    int bullet_puller       ( player*, item*, bool, const tripoint& );
    int boltcutters         ( player*, item*, bool, const tripoint& );
    int mop                 ( player*, item*, bool, const tripoint& );
    int spray_can           ( player*, item*, bool, const tripoint& );
    int rag                 ( player*, item*, bool, const tripoint& );
    int LAW                 ( player*, item*, bool, const tripoint& );
    int heatpack            ( player*, item*, bool, const tripoint& );
    int hotplate            ( player*, item*, bool, const tripoint& );
    int quiver              ( player*, item*, bool, const tripoint& );
    int towel               ( player*, item*, bool, const tripoint& );
    int unfold_generic      ( player*, item*, bool, const tripoint& );
    int adrenaline_injector ( player*, item*, bool, const tripoint& );
    int jet_injector        ( player*, item*, bool, const tripoint& );
    int stimpack            ( player*, item*, bool, const tripoint& );
    int contacts            ( player*, item*, bool, const tripoint& );
    int talking_doll        ( player*, item*, bool, const tripoint& );
    int bell                ( player*, item*, bool, const tripoint& );
    int seed                ( player*, item*, bool, const tripoint& );
    int oxygen_bottle       ( player*, item*, bool, const tripoint& );
    int atomic_battery      ( player*, item*, bool, const tripoint& );
    int ups_battery         ( player*, item*, bool, const tripoint& );
    int radio_mod           ( player*, item*, bool, const tripoint& );
    int remove_all_mods     ( player*, item*, bool, const tripoint& );
    int fishing_rod         ( player*, item*, bool, const tripoint& );
    int fish_trap           ( player*, item*, bool, const tripoint& );
    int gun_repair          ( player*, item*, bool, const tripoint& );
    int misc_repair         ( player*, item*, bool, const tripoint& );
    int rm13armor_off       ( player*, item*, bool, const tripoint& );
    int rm13armor_on        ( player*, item*, bool, const tripoint& );
    int unpack_item         ( player*, item*, bool, const tripoint& );
    int pack_item           ( player*, item*, bool, const tripoint& );
    int radglove            ( player*, item*, bool, const tripoint& );
    int robotcontrol        ( player*, item*, bool, const tripoint& );
    int einktabletpc        ( player*, item*, bool, const tripoint& );
    int camera              ( player*, item*, bool, const tripoint& );
    int ehandcuffs          ( player*, item*, bool, const tripoint& );
    int cable_attach        ( player*, item*, bool, const tripoint& );
    int shavekit            ( player*, item*, bool, const tripoint& );
    int hairkit             ( player*, item*, bool, const tripoint& );
    int weather_tool        ( player*, item*, bool, const tripoint& );
    int ladder              ( player*, item*, bool, const tripoint& );

// MACGUFFINS
    int mcg_note            ( player*, item*, bool, const tripoint& );

    int radiocar( player*, item*, bool, const tripoint& );
    int radiocaron( player*, item*, bool, const tripoint& );
    int radiocontrol( player*, item*, bool, const tripoint& );

    int multicooker( player*, item*, bool, const tripoint& );

    int remoteveh( player*, item*, bool, const tripoint& );

// ARTIFACTS
    /* This function is used when an artifact is activated.
       It examines the item's artifact-specific properties.
       See artifact.h for a list.                        */
    int artifact            ( player*, item*, bool, const tripoint& );

    // Helper for listening to music, might deserve a better home, but not sure where.
    static void play_music( player *p, const tripoint &source, int volume, int max_morale );

    // Helper for handling pesky wannabe-artists
    static int handle_ground_graffiti( player *p, item *it, const std::string prefix );

    static void reset_bullet_pulling();
    static void load_bullet_pulling(JsonObject &jo);
protected:
    typedef std::pair<std::string, int> result_t;
    typedef std::vector<result_t> result_list_t;
    typedef std::map<std::string, result_list_t> bullet_pulling_t;
    static bullet_pulling_t bullet_pulling_recipes;
};


typedef int (iuse::*use_function_pointer)( player*, item*, bool, const tripoint& );

class iuse_actor {
protected:
    iuse_actor() { }
public:
    std::string type;
    virtual ~iuse_actor() { }
    virtual long use( player*, item*, bool, const tripoint& ) const = 0;
    virtual bool can_use( const player*, const item*, bool, const tripoint& ) const { return true; }
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

    long call( player*,item*,bool, const tripoint& ) const;

    iuse_actor *get_actor_ptr() const
    {
        if( function_type != USE_FUNCTION_ACTOR_PTR ) {
            return nullptr;
        }
        return actor_ptr;
    }

    // Gets actor->type or finds own type in item_factory::iuse_function_list
    std::string get_type_name() const;
    // Returns translated name of the action
    std::string get_name() const;

    bool can_call(const player *p, const item *it, bool t, const tripoint &pos) const
    {
        auto actor = get_actor_ptr();
        return actor == nullptr || actor->can_use( p, it, t, pos );
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
