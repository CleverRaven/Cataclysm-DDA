#pragma once
#ifndef CATA_SRC_IUSE_H
#define CATA_SRC_IUSE_H

#include <iosfwd>
#include <memory>
#include <type_traits>
#include <vector>

#include "clone_ptr.h"
#include "optional.h"
#include "type_id.h"
#include "units_fwd.h"

class Character;
class JsonObject;
class item;
class monster;
class player;
struct iteminfo;
struct tripoint;
template<typename T> class ret_val;

// iuse methods returning a bool indicating whether to consume a charge of the item being used.
namespace iuse
{

// FOOD AND DRUGS (ADMINISTRATION)
cata::optional<int> alcohol_medium( player *, item *, bool, const tripoint & );
cata::optional<int> alcohol_strong( player *, item *, bool, const tripoint & );
cata::optional<int> alcohol_weak( player *, item *, bool, const tripoint & );
cata::optional<int> antibiotic( player *, item *, bool, const tripoint & );
cata::optional<int> anticonvulsant( player *, item *, bool, const tripoint & );
cata::optional<int> antifungal( player *, item *, bool, const tripoint & );
cata::optional<int> antiparasitic( player *, item *, bool, const tripoint & );
cata::optional<int> blech( player *, item *, bool, const tripoint & );
cata::optional<int> blech_because_unclean( player *, item *, bool, const tripoint & );
cata::optional<int> catfood( player *, item *, bool, const tripoint & );
cata::optional<int> chew( player *, item *, bool, const tripoint & );
cata::optional<int> coke( player *, item *, bool, const tripoint & );
cata::optional<int> datura( player *, item *, bool, const tripoint & );
cata::optional<int> dogfood( player *, item *, bool, const tripoint & );
cata::optional<int> ecig( player *, item *, bool, const tripoint & );
cata::optional<int> eyedrops( player *, item *, bool, const tripoint & );
cata::optional<int> feedbird( player *, item *, bool, const tripoint & );
cata::optional<int> feedcattle( player *, item *, bool, const tripoint & );
cata::optional<int> flu_vaccine( player *, item *, bool, const tripoint & );
cata::optional<int> flumed( player *, item *, bool, const tripoint & );
cata::optional<int> flusleep( player *, item *, bool, const tripoint & );
cata::optional<int> fungicide( player *, item *, bool, const tripoint & );
cata::optional<int> honeycomb( player *, item *, bool, const tripoint & );
cata::optional<int> inhaler( player *, item *, bool, const tripoint & );
cata::optional<int> marloss( player *, item *, bool, const tripoint & );
cata::optional<int> marloss_gel( player *, item *, bool, const tripoint & );
cata::optional<int> marloss_seed( player *, item *, bool, const tripoint & );
cata::optional<int> meditate( player *, item *, bool, const tripoint & );
cata::optional<int> meth( player *, item *, bool, const tripoint & );
cata::optional<int> mycus( player *, item *, bool, const tripoint & );
cata::optional<int> plantblech( player *, item *, bool, const tripoint & );
cata::optional<int> poison( player *, item *, bool, const tripoint & );
cata::optional<int> prozac( player *, item *, bool, const tripoint & );
cata::optional<int> purifier( player *, item *, bool, const tripoint & );
cata::optional<int> purify_iv( player *, item *, bool, const tripoint & );
cata::optional<int> purify_smart( player *, item *, bool, const tripoint & );
cata::optional<int> sewage( player *, item *, bool, const tripoint & );
cata::optional<int> smoking( player *, item *, bool, const tripoint & );
cata::optional<int> thorazine( player *, item *, bool, const tripoint & );
cata::optional<int> vaccine( player *, item *, bool, const tripoint & );
cata::optional<int> weed_cake( player *, item *, bool, const tripoint & );
cata::optional<int> xanax( player *, item *, bool, const tripoint & );

// TOOLS
cata::optional<int> acidbomb_act( player *, item *, bool, const tripoint & );
cata::optional<int> adrenaline_injector( player *, item *, bool, const tripoint & );
cata::optional<int> arrow_flammable( player *, item *, bool, const tripoint & );
cata::optional<int> bell( player *, item *, bool, const tripoint & );
cata::optional<int> blood_draw( player *, item *, bool, const tripoint & );
cata::optional<int> boltcutters( player *, item *, bool, const tripoint & );
cata::optional<int> break_stick( player *, item *, bool, const tripoint & );
cata::optional<int> c4( player *, item *, bool, const tripoint & );
cata::optional<int> cable_attach( player *, item *, bool, const tripoint & );
cata::optional<int> call_of_tindalos( player *, item *, bool, const tripoint & );
cata::optional<int> camera( player *, item *, bool, const tripoint & );
cata::optional<int> can_goo( player *, item *, bool, const tripoint & );
cata::optional<int> capture_monster_act( player *, item *, bool, const tripoint & );
cata::optional<int> capture_monster_veh( player *, item *, bool, const tripoint & );
cata::optional<int> carver_off( player *, item *, bool, const tripoint & );
cata::optional<int> carver_on( player *, item *, bool, const tripoint & );
cata::optional<int> chainsaw_off( player *, item *, bool, const tripoint & );
cata::optional<int> chainsaw_on( player *, item *, bool, const tripoint & );
cata::optional<int> chop_logs( player *, item *, bool, const tripoint & );
cata::optional<int> chop_tree( player *, item *, bool, const tripoint & );
cata::optional<int> circsaw_on( player *, item *, bool, const tripoint & );
cata::optional<int> clear_rubble( player *, item *, bool, const tripoint & );
cata::optional<int> coin_flip( player *, item *, bool, const tripoint & );
cata::optional<int> combatsaw_off( player *, item *, bool, const tripoint & );
cata::optional<int> combatsaw_on( player *, item *, bool, const tripoint & );
cata::optional<int> contacts( player *, item *, bool, const tripoint & );
cata::optional<int> crowbar( player *, item *, bool, const tripoint & );
cata::optional<int> cs_lajatang_off( player *, item *, bool, const tripoint & );
cata::optional<int> cs_lajatang_on( player *, item *, bool, const tripoint & );
cata::optional<int> dig( player *, item *, bool, const tripoint & );
cata::optional<int> dig_channel( player *, item *, bool, const tripoint & );
cata::optional<int> directional_antenna( player *, item *, bool, const tripoint & );
cata::optional<int> directional_hologram( player *, item *, bool, const tripoint & );
cata::optional<int> dive_tank( player *, item *, bool, const tripoint & );
cata::optional<int> dog_whistle( player *, item *, bool, const tripoint & );
cata::optional<int> e_combatsaw_off( player *, item *, bool, const tripoint & );
cata::optional<int> e_combatsaw_on( player *, item *, bool, const tripoint & );
cata::optional<int> ecs_lajatang_off( player *, item *, bool, const tripoint & );
cata::optional<int> ecs_lajatang_on( player *, item *, bool, const tripoint & );
cata::optional<int> ehandcuffs( player *, item *, bool, const tripoint & );
cata::optional<int> einktabletpc( player *, item *, bool, const tripoint & );
cata::optional<int> elec_chainsaw_off( player *, item *, bool, const tripoint & );
cata::optional<int> elec_chainsaw_on( player *, item *, bool, const tripoint & );
cata::optional<int> extinguisher( player *, item *, bool, const tripoint & );
cata::optional<int> fill_pit( player *, item *, bool, const tripoint & );
cata::optional<int> firecracker( player *, item *, bool, const tripoint & );
cata::optional<int> firecracker_act( player *, item *, bool, const tripoint & );
cata::optional<int> firecracker_pack( player *, item *, bool, const tripoint & );
cata::optional<int> firecracker_pack_act( player *, item *, bool, const tripoint & );
cata::optional<int> fish_trap( player *, item *, bool, const tripoint & );
cata::optional<int> fishing_rod( player *, item *, bool, const tripoint & );
cata::optional<int> fitness_check( player *p, item *it, bool, const tripoint & );
cata::optional<int> foodperson( player *, item *, bool, const tripoint & );
cata::optional<int> gasmask( player *, item *, bool, const tripoint & );
cata::optional<int> geiger( player *, item *, bool, const tripoint & );
cata::optional<int> granade( player *, item *, bool, const tripoint & );
cata::optional<int> granade_act( player *, item *, bool, const tripoint & );
cata::optional<int> grenade_inc_act( player *, item *, bool, const tripoint & );
cata::optional<int> gun_repair( player *, item *, bool, const tripoint & );
cata::optional<int> gunmod_attach( player *, item *, bool, const tripoint & );
cata::optional<int> hacksaw( player *, item *, bool, const tripoint & );
cata::optional<int> hairkit( player *, item *, bool, const tripoint & );
cata::optional<int> hammer( player *, item *, bool, const tripoint & );
cata::optional<int> hand_crank( player *, item *, bool, const tripoint & );
cata::optional<int> heat_food( player *, item *, bool, const tripoint & );
cata::optional<int> heatpack( player *, item *, bool, const tripoint & );
cata::optional<int> hotplate( player *, item *, bool, const tripoint & );
cata::optional<int> jackhammer( player *, item *, bool, const tripoint & );
cata::optional<int> jet_injector( player *, item *, bool, const tripoint & );
cata::optional<int> ladder( player *, item *, bool, const tripoint & );
cata::optional<int> lumber( player *, item *, bool, const tripoint & );
cata::optional<int> ma_manual( player *, item *, bool, const tripoint & );
cata::optional<int> magic_8_ball( player *, item *, bool, const tripoint & );
cata::optional<int> makemound( player *, item *, bool, const tripoint & );
cata::optional<int> melatonin_tablet( player *, item *, bool, const tripoint & );
cata::optional<int> mind_splicer( player *, item *, bool, const tripoint & );
cata::optional<int> mininuke( player *, item *, bool, const tripoint & );
cata::optional<int> molotov_lit( player *, item *, bool, const tripoint & );
cata::optional<int> mop( player *, item *, bool, const tripoint & );
cata::optional<int> mp3( player *, item *, bool, const tripoint & );
cata::optional<int> mp3_on( player *, item *, bool, const tripoint & );
cata::optional<int> noise_emitter_off( player *, item *, bool, const tripoint & );
cata::optional<int> noise_emitter_on( player *, item *, bool, const tripoint & );
cata::optional<int> oxygen_bottle( player *, item *, bool, const tripoint & );
cata::optional<int> oxytorch( player *, item *, bool, const tripoint & );
cata::optional<int> pack_cbm( player *p, item *it, bool, const tripoint & );
cata::optional<int> pack_item( player *, item *, bool, const tripoint & );
cata::optional<int> pick_lock( player *p, item *it, bool, const tripoint &pos );
cata::optional<int> pickaxe( player *, item *, bool, const tripoint & );
cata::optional<int> play_game( player *, item *, bool, const tripoint & );
cata::optional<int> portable_game( player *, item *, bool active, const tripoint & );
cata::optional<int> portal( player *, item *, bool, const tripoint & );
cata::optional<int> radglove( player *, item *, bool, const tripoint & );
cata::optional<int> radio_mod( player *, item *, bool, const tripoint & );
cata::optional<int> radio_off( player *, item *, bool, const tripoint & );
cata::optional<int> radio_on( player *, item *, bool, const tripoint & );
cata::optional<int> remove_all_mods( player *, item *, bool, const tripoint & );
cata::optional<int> rm13armor_off( player *, item *, bool, const tripoint & );
cata::optional<int> rm13armor_on( player *, item *, bool, const tripoint & );
cata::optional<int> robotcontrol( player *, item *, bool active, const tripoint & );
cata::optional<int> rpgdie( player *, item *, bool, const tripoint & );
cata::optional<int> seed( player *, item *, bool, const tripoint & );
cata::optional<int> shavekit( player *, item *, bool, const tripoint & );
cata::optional<int> shocktonfa_off( player *, item *, bool, const tripoint & );
cata::optional<int> shocktonfa_on( player *, item *, bool, const tripoint & );
cata::optional<int> siphon( player *, item *, bool, const tripoint & );
cata::optional<int> solarpack( player *, item *, bool, const tripoint & );
cata::optional<int> solarpack_off( player *, item *, bool, const tripoint & );
cata::optional<int> spray_can( player *, item *, bool, const tripoint & );
cata::optional<int> stimpack( player *, item *, bool, const tripoint & );
cata::optional<int> strong_antibiotic( player *, item *, bool, const tripoint & );
cata::optional<int> talking_doll( player *, item *, bool, const tripoint & );
cata::optional<int> tazer( player *, item *, bool, const tripoint & );
cata::optional<int> tazer2( player *, item *, bool, const tripoint & );
cata::optional<int> teleport( player *, item *, bool, const tripoint & );
cata::optional<int> toolmod_attach( player *, item *, bool, const tripoint & );
cata::optional<int> tow_attach( player *, item *, bool, const tripoint & );
cata::optional<int> towel( player *, item *, bool, const tripoint & );
cata::optional<int> trimmer_off( player *, item *, bool, const tripoint & );
cata::optional<int> trimmer_on( player *, item *, bool, const tripoint & );
cata::optional<int> unfold_generic( player *, item *, bool, const tripoint & );
cata::optional<int> unpack_item( player *, item *, bool, const tripoint & );
cata::optional<int> vibe( player *, item *, bool, const tripoint & );
cata::optional<int> vortex( player *, item *, bool, const tripoint & );
cata::optional<int> wash_all_items( player *, item *, bool, const tripoint & );
cata::optional<int> wash_hard_items( player *, item *, bool, const tripoint & );
cata::optional<int> wash_items( player *p, bool soft_items, bool hard_items );
cata::optional<int> wash_soft_items( player *, item *, bool, const tripoint & );
cata::optional<int> water_purifier( player *, item *, bool, const tripoint & );
cata::optional<int> weak_antibiotic( player *, item *, bool, const tripoint & );
cata::optional<int> weather_tool( player *, item *, bool, const tripoint & );

// MACGUFFINS

cata::optional<int> radiocar( player *, item *, bool, const tripoint & );
cata::optional<int> radiocaron( player *, item *, bool, const tripoint & );
cata::optional<int> radiocontrol( player *, item *, bool, const tripoint & );

cata::optional<int> autoclave( player *, item *, bool, const tripoint & );

cata::optional<int> multicooker( player *, item *, bool, const tripoint & );

cata::optional<int> remoteveh( player *, item *, bool, const tripoint & );

cata::optional<int> craft( player *, item *, bool, const tripoint & );

cata::optional<int> disassemble( player *, item *, bool, const tripoint & );

// Helper functions for other iuse functions
void cut_log_into_planks( Character & );
void play_music( Character &p, const tripoint &source, int volume, int max_morale );
int towel_common( Character *, item *, bool );

// Helper for validating a potential taget of robot control
bool robotcontrol_can_target( player *, const monster & );

// Helper for handling pesky wannabe-artists
cata::optional<int> handle_ground_graffiti( Character &p, item *it, const std::string &prefix,
        const tripoint &where );

//helper for lit cigs
cata::optional<std::string> can_smoke( const player &u );
} // namespace iuse

void remove_radio_mod( item &it, Character &p );

// Helper for clothes washing
struct washing_requirements {
    int water;
    int cleanser;
    int time;
};
washing_requirements washing_requirements_for_volume( const units::volume & );

using use_function_pointer = cata::optional<int> ( * )( player *, item *, bool, const tripoint & );

class iuse_actor
{
    protected:
        explicit iuse_actor( const std::string &type, int cost = -1 ) : type( type ), cost( cost ) {}

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
        virtual cata::optional<int> use( player &, item &, bool, const tripoint & ) const = 0;
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
        explicit use_function( std::unique_ptr<iuse_actor> f ) : actor( std::move( f ) ) {}

        cata::optional<int> call( player &, item &, bool, const tripoint & ) const;
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
