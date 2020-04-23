#pragma once
#ifndef CATA_SRC_IUSE_H
#define CATA_SRC_IUSE_H

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "clone_ptr.h"
#include "units.h"

class JsonObject;
class item;
class monster;
class Character;
struct iteminfo;
template<typename T> class ret_val;

using itype_id = std::string;
struct tripoint;

// iuse methods returning a bool indicating whether to consume a charge of the item being used.
class iuse
{
    public:
        // FOOD AND DRUGS (ADMINISTRATION)
        int sewage( Character *, item *, bool, const tripoint & );
        int honeycomb( Character *, item *, bool, const tripoint & );
        int royal_jelly( Character *, item *, bool, const tripoint & );
        int alcohol_weak( Character *, item *, bool, const tripoint & );
        int alcohol_medium( Character *, item *, bool, const tripoint & );
        int alcohol_strong( Character *, item *, bool, const tripoint & );
        int xanax( Character *, item *, bool, const tripoint & );
        int smoking( Character *, item *, bool, const tripoint & );
        int ecig( Character *, item *, bool, const tripoint & );
        int antibiotic( Character *, item *, bool, const tripoint & );
        int eyedrops( Character *, item *, bool, const tripoint & );
        int fungicide( Character *, item *, bool, const tripoint & );
        int antifungal( Character *, item *, bool, const tripoint & );
        int antiparasitic( Character *, item *, bool, const tripoint & );
        int anticonvulsant( Character *, item *, bool, const tripoint & );
        int weed_cake( Character *, item *, bool, const tripoint & );
        int coke( Character *, item *, bool, const tripoint & );
        int meth( Character *, item *, bool, const tripoint & );
        int vaccine( Character *, item *, bool, const tripoint & );
        int flu_vaccine( Character *, item *, bool, const tripoint & );
        int poison( Character *, item *, bool, const tripoint & );
        int meditate( Character *, item *, bool, const tripoint & );
        int thorazine( Character *, item *, bool, const tripoint & );
        int prozac( Character *, item *, bool, const tripoint & );
        int sleep( Character *, item *, bool, const tripoint & );
        int datura( Character *, item *, bool, const tripoint & );
        int flumed( Character *, item *, bool, const tripoint & );
        int flusleep( Character *, item *, bool, const tripoint & );
        int inhaler( Character *, item *, bool, const tripoint & );
        int blech( Character *, item *, bool, const tripoint & );
        int blech_because_unclean( Character *, item *, bool, const tripoint & );
        int plantblech( Character *, item *, bool, const tripoint & );
        int chew( Character *, item *, bool, const tripoint & );
        int purifier( Character *, item *, bool, const tripoint & );
        int purify_iv( Character *, item *, bool, const tripoint & );
        int purify_smart( Character *, item *, bool, const tripoint & );
        int marloss( Character *, item *, bool, const tripoint & );
        int marloss_seed( Character *, item *, bool, const tripoint & );
        int marloss_gel( Character *, item *, bool, const tripoint & );
        int mycus( Character *, item *, bool, const tripoint & );
        int dogfood( Character *, item *, bool, const tripoint & );
        int catfood( Character *, item *, bool, const tripoint & );
        int feedcattle( Character *, item *, bool, const tripoint & );
        int feedbird( Character *, item *, bool, const tripoint & );
        // TOOLS
        int extinguisher( Character *, item *, bool, const tripoint & );
        int hammer( Character *, item *, bool, const tripoint & );
        int water_purifier( Character *, item *, bool, const tripoint & );
        int directional_antenna( Character *, item *, bool, const tripoint & );
        int radio_off( Character *, item *, bool, const tripoint & );
        int radio_on( Character *, item *, bool, const tripoint & );
        int noise_emitter_off( Character *, item *, bool, const tripoint & );
        int noise_emitter_on( Character *, item *, bool, const tripoint & );
        int ma_manual( Character *, item *, bool, const tripoint & );
        int crowbar( Character *, item *, bool, const tripoint & );
        int makemound( Character *, item *, bool, const tripoint & );
        int dig( Character *, item *, bool, const tripoint & );
        int dig_channel( Character *, item *, bool, const tripoint & );
        int fill_pit( Character *, item *, bool, const tripoint & );
        int clear_rubble( Character *, item *, bool, const tripoint & );
        int siphon( Character *, item *, bool, const tripoint & );
        int chainsaw_off( Character *, item *, bool, const tripoint & );
        int chainsaw_on( Character *, item *, bool, const tripoint & );
        int elec_chainsaw_off( Character *, item *, bool, const tripoint & );
        int elec_chainsaw_on( Character *, item *, bool, const tripoint & );
        int cs_lajatang_off( Character *, item *, bool, const tripoint & );
        int cs_lajatang_on( Character *, item *, bool, const tripoint & );
        int ecs_lajatang_off( Character *, item *, bool, const tripoint & );
        int ecs_lajatang_on( Character *, item *, bool, const tripoint & );
        int carver_off( Character *, item *, bool, const tripoint & );
        int carver_on( Character *, item *, bool, const tripoint & );
        int trimmer_off( Character *, item *, bool, const tripoint & );
        int trimmer_on( Character *, item *, bool, const tripoint & );
        int circsaw_on( Character *, item *, bool, const tripoint & );
        int combatsaw_off( Character *, item *, bool, const tripoint & );
        int combatsaw_on( Character *, item *, bool, const tripoint & );
        int e_combatsaw_off( Character *, item *, bool, const tripoint & );
        int e_combatsaw_on( Character *, item *, bool, const tripoint & );
        int jackhammer( Character *, item *, bool, const tripoint & );
        int pickaxe( Character *, item *, bool, const tripoint & );
        int burrow( Character *, item *, bool, const tripoint & );
        int geiger( Character *, item *, bool, const tripoint & );
        int teleport( Character *, item *, bool, const tripoint & );
        int can_goo( Character *, item *, bool, const tripoint & );
        int throwable_extinguisher_act( Character *, item *, bool, const tripoint & );
        int directional_hologram( Character *, item *, bool, const tripoint & );
        int capture_monster_veh( Character *, item *, bool, const tripoint & );
        int capture_monster_act( Character *, item *, bool, const tripoint & );
        int granade( Character *, item *, bool, const tripoint & );
        int granade_act( Character *, item *, bool, const tripoint & );
        int c4( Character *, item *, bool, const tripoint & );
        int arrow_flammable( Character *, item *, bool, const tripoint & );
        int acidbomb_act( Character *, item *, bool, const tripoint & );
        int grenade_inc_act( Character *, item *, bool, const tripoint & );
        int molotov_lit( Character *, item *, bool, const tripoint & );
        int firecracker_pack( Character *, item *, bool, const tripoint & );
        int firecracker_pack_act( Character *, item *, bool, const tripoint & );
        int firecracker( Character *, item *, bool, const tripoint & );
        int firecracker_act( Character *, item *, bool, const tripoint & );
        int mininuke( Character *, item *, bool, const tripoint & );
        int pheromone( Character *, item *, bool, const tripoint & );
        int portal( Character *, item *, bool, const tripoint & );
        int tazer( Character *, item *, bool, const tripoint & );
        int tazer2( Character *, item *, bool, const tripoint & );
        int shocktonfa_off( Character *, item *, bool, const tripoint & );
        int shocktonfa_on( Character *, item *, bool, const tripoint & );
        int mp3( Character *, item *, bool, const tripoint & );
        int mp3_on( Character *, item *, bool, const tripoint & );
        int rpgdie( Character *, item *, bool, const tripoint & );
        int dive_tank( Character *, item *, bool, const tripoint & );
        int gasmask( Character *, item *, bool, const tripoint & );
        int portable_game( Character *, item *, bool, const tripoint & );
        int fitness_check( Character *p, item *it, bool, const tripoint & );
        int vibe( Character *, item *, bool, const tripoint & );
        int hand_crank( Character *, item *, bool, const tripoint & );
        int vortex( Character *, item *, bool, const tripoint & );
        int dog_whistle( Character *, item *, bool, const tripoint & );
        int call_of_tindalos( Character *, item *, bool, const tripoint & );
        int blood_draw( Character *, item *, bool, const tripoint & );
        int mind_splicer( Character *, item *, bool, const tripoint & );
        static void cut_log_into_planks( Character & );
        int lumber( Character *, item *, bool, const tripoint & );
        int chop_tree( Character *, item *, bool, const tripoint & );
        int chop_logs( Character *, item *, bool, const tripoint & );
        int oxytorch( Character *, item *, bool, const tripoint & );
        int hacksaw( Character *, item *, bool, const tripoint & );
        int boltcutters( Character *, item *, bool, const tripoint & );
        int mop( Character *, item *, bool, const tripoint & );
        int spray_can( Character *, item *, bool, const tripoint & );
        int heatpack( Character *, item *, bool, const tripoint & );
        int heat_food( Character *, item *, bool, const tripoint & );
        int hotplate( Character *, item *, bool, const tripoint & );
        int towel( Character *, item *, bool, const tripoint & );
        int unfold_generic( Character *, item *, bool, const tripoint & );
        int adrenaline_injector( Character *, item *, bool, const tripoint & );
        int jet_injector( Character *, item *, bool, const tripoint & );
        int stimpack( Character *, item *, bool, const tripoint & );
        int contacts( Character *, item *, bool, const tripoint & );
        int talking_doll( Character *, item *, bool, const tripoint & );
        int bell( Character *, item *, bool, const tripoint & );
        int seed( Character *, item *, bool, const tripoint & );
        int oxygen_bottle( Character *, item *, bool, const tripoint & );
        int radio_mod( Character *, item *, bool, const tripoint & );
        int remove_all_mods( Character *, item *, bool, const tripoint & );
        int fishing_rod( Character *, item *, bool, const tripoint & );
        int fish_trap( Character *, item *, bool, const tripoint & );
        int gun_repair( Character *, item *, bool, const tripoint & );
        int gunmod_attach( Character *, item *, bool, const tripoint & );
        int toolmod_attach( Character *, item *, bool, const tripoint & );
        int rm13armor_off( Character *, item *, bool, const tripoint & );
        int rm13armor_on( Character *, item *, bool, const tripoint & );
        int unpack_item( Character *, item *, bool, const tripoint & );
        int pack_cbm( Character *p, item *it, bool, const tripoint & );
        int pack_item( Character *, item *, bool, const tripoint & );
        int radglove( Character *, item *, bool, const tripoint & );
        int robotcontrol( Character *, item *, bool, const tripoint & );
        // Helper for validating a potential taget of robot control
        static bool robotcontrol_can_target( Character *, const monster & );
        int einktabletpc( Character *, item *, bool, const tripoint & );
        int camera( Character *, item *, bool, const tripoint & );
        int ehandcuffs( Character *, item *, bool, const tripoint & );
        int foodperson( Character *, item *, bool, const tripoint & );
        int tow_attach( Character *, item *, bool, const tripoint & );
        int cable_attach( Character *, item *, bool, const tripoint & );
        int shavekit( Character *, item *, bool, const tripoint & );
        int hairkit( Character *, item *, bool, const tripoint & );
        int weather_tool( Character *, item *, bool, const tripoint & );
        int ladder( Character *, item *, bool, const tripoint & );
        int wash_soft_items( Character *, item *, bool, const tripoint & );
        int wash_hard_items( Character *, item *, bool, const tripoint & );
        int wash_all_items( Character *, item *, bool, const tripoint & );
        int wash_items( Character *p, bool soft_items, bool hard_items );
        int solarpack( Character *, item *, bool, const tripoint & );
        int solarpack_off( Character *, item *, bool, const tripoint & );
        int break_stick( Character *, item *, bool, const tripoint & );
        int weak_antibiotic( Character *, item *, bool, const tripoint & );
        int strong_antibiotic( Character *, item *, bool, const tripoint & );
        int panacea( Character *, item *, bool, const tripoint & );
        int melatonin_tablet( Character *, item *, bool, const tripoint & );
        int coin_flip( Character *, item *, bool, const tripoint & );
        int play_game( Character *, item *, bool, const tripoint & );
        int magic_8_ball( Character *, item *, bool, const tripoint & );

        // MACGUFFINS

        int radiocar( Character *, item *, bool, const tripoint & );
        int radiocaron( Character *, item *, bool, const tripoint & );
        int radiocontrol( Character *, item *, bool, const tripoint & );

        int autoclave( Character *, item *, bool, const tripoint & );

        int multicooker( Character *, item *, bool, const tripoint & );

        int remoteveh( Character *, item *, bool, const tripoint & );

        int craft( Character *, item *, bool, const tripoint & );

        int disassemble( Character *, item *, bool, const tripoint & );

        // ARTIFACTS
        /* This function is used when an artifact is activated.
           It examines the item's artifact-specific properties.
           See artifact.h for a list.                        */
        int artifact( Character *, item *, bool, const tripoint & );

        // Helper for listening to music, might deserve a better home, but not sure where.
        static void play_music( Character &p, const tripoint &source, int volume, int max_morale );
        static int towel_common( Character *, item *, bool );

        // Helper for handling pesky wannabe-artists
        static int handle_ground_graffiti( Character &guy, item *it, const std::string &prefix,
                                           const tripoint &where );

};

void remove_radio_mod( item &it, Character &guy );

// Helper for clothes washing
struct washing_requirements {
    int water;
    int cleanser;
    int time;
};
washing_requirements washing_requirements_for_volume( const units::volume & );

using use_function_pointer = int ( iuse::* )( Character *, item *, bool, const tripoint & );

class iuse_actor
{
    protected:
        iuse_actor( const std::string &type, int cost = -1 ) : type( type ), cost( cost ) {}

    public:
        /**
         * The type of the action. It's not translated. Different iuse_actor instances may have the
         * same type, but different data.
         */
        const std::string type;

        /** Units of ammo required per invocation (or use value from base item if negative) */
        int cost;

        virtual ~iuse_actor() = default;
        virtual void load( const JsonObject &jo ) = 0;
        virtual int use( Character &, item &, bool, const tripoint & ) const = 0;
        virtual ret_val<bool> can_use( const Character &, const item &, bool, const tripoint & ) const;
        virtual void info( const item &, std::vector<iteminfo> & ) const {}
        /**
         * Returns a deep copy of this object. Example implementation:
         * \code
         * class my_iuse_actor {
         *     std::unique_ptr<iuse_actor> clone() const override {
         *         return std::make_unique<my_iuse_actor>( *this );
         *     }
         * };
         * \endcode
         * The returned value should behave like the original item and must have the same type.
         */
        virtual std::unique_ptr<iuse_actor> clone() const = 0;
        /**
         * Returns whether the actor is valid (exists in the generator).
         */
        virtual bool is_valid() const;
        /**
         * Returns the translated name of the action. It is used for the item action menu.
         */
        virtual std::string get_name() const;
        /**
         * Finalizes the actor. Must be called after all items are loaded.
         */
        virtual void finalize( const itype_id &/*my_item_type*/ ) { }
};

struct use_function {
    protected:
        cata::clone_ptr<iuse_actor> actor;

    public:
        use_function() = default;
        use_function( const std::string &type, use_function_pointer f );
        use_function( std::unique_ptr<iuse_actor> f ) : actor( std::move( f ) ) {}

        int call( Character &, item &, bool, const tripoint & ) const;
        ret_val<bool> can_call( const Character &, const item &, bool t, const tripoint &pos ) const;

        iuse_actor *get_actor_ptr() {
            return actor.get();
        }

        const iuse_actor *get_actor_ptr() const {
            return actor.get();
        }

        explicit operator bool() const {
            return actor != nullptr;
        }

        /** @return See @ref iuse_actor::type */
        std::string get_type() const;
        /** @return See @ref iuse_actor::get_name */
        std::string get_name() const;
        /** @return Used by @ref item::info to get description of the actor */
        void dump_info( const item &, std::vector<iteminfo> & ) const;
};

#endif // CATA_SRC_IUSE_H
