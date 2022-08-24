#pragma once
#ifndef CATA_SRC_IUSE_H
#define CATA_SRC_IUSE_H

#include <iosfwd>
#include <memory>
#include <type_traits>
#include <vector>

#include "clone_ptr.h"
#include "item_location.h"
#include "optional.h"
#include "type_id.h"
#include "units_fwd.h"

class Character;
class JsonObject;
class item;
class monster;
struct iteminfo;
struct tripoint;
template<typename T> class ret_val;

// iuse methods returning a bool indicating whether to consume a charge of the item being used.
namespace iuse
{

// FOOD AND DRUGS (ADMINISTRATION)
cata::optional<int> alcohol_medium( Character *, item *, bool, const tripoint & );
cata::optional<int> alcohol_strong( Character *, item *, bool, const tripoint & );
cata::optional<int> alcohol_weak( Character *, item *, bool, const tripoint & );
cata::optional<int> antibiotic( Character *, item *, bool, const tripoint & );
cata::optional<int> anticonvulsant( Character *, item *, bool, const tripoint & );
cata::optional<int> antifungal( Character *, item *, bool, const tripoint & );
cata::optional<int> antiparasitic( Character *, item *, bool, const tripoint & );
cata::optional<int> blech( Character *, item *, bool, const tripoint & );
cata::optional<int> blech_because_unclean( Character *, item *, bool, const tripoint & );
cata::optional<int> chew( Character *, item *, bool, const tripoint & );
cata::optional<int> coke( Character *, item *, bool, const tripoint & );
cata::optional<int> datura( Character *, item *, bool, const tripoint & );
cata::optional<int> ecig( Character *, item *, bool, const tripoint & );
cata::optional<int> eyedrops( Character *, item *, bool, const tripoint & );
cata::optional<int> flu_vaccine( Character *, item *, bool, const tripoint & );
cata::optional<int> flumed( Character *, item *, bool, const tripoint & );
cata::optional<int> flusleep( Character *, item *, bool, const tripoint & );
cata::optional<int> fungicide( Character *, item *, bool, const tripoint & );
cata::optional<int> honeycomb( Character *, item *, bool, const tripoint & );
cata::optional<int> inhaler( Character *, item *, bool, const tripoint & );
cata::optional<int> marloss( Character *, item *, bool, const tripoint & );
cata::optional<int> marloss_gel( Character *, item *, bool, const tripoint & );
cata::optional<int> marloss_seed( Character *, item *, bool, const tripoint & );
cata::optional<int> meditate( Character *, item *, bool, const tripoint & );
cata::optional<int> meth( Character *, item *, bool, const tripoint & );
cata::optional<int> mycus( Character *, item *, bool, const tripoint & );
cata::optional<int> petfood( Character *p, item *it, bool, const tripoint & );
cata::optional<int> plantblech( Character *, item *, bool, const tripoint & );
cata::optional<int> poison( Character *, item *, bool, const tripoint & );
cata::optional<int> prozac( Character *, item *, bool, const tripoint & );
cata::optional<int> purify_smart( Character *, item *, bool, const tripoint & );
cata::optional<int> sewage( Character *, item *, bool, const tripoint & );
cata::optional<int> smoking( Character *, item *, bool, const tripoint & );
cata::optional<int> thorazine( Character *, item *, bool, const tripoint & );
cata::optional<int> vaccine( Character *, item *, bool, const tripoint & );
cata::optional<int> weed_cake( Character *, item *, bool, const tripoint & );
cata::optional<int> xanax( Character *, item *, bool, const tripoint & );

// TOOLS
cata::optional<int> acidbomb_act( Character *, item *, bool, const tripoint & );
cata::optional<int> adrenaline_injector( Character *, item *, bool, const tripoint & );
cata::optional<int> bell( Character *, item *, bool, const tripoint & );
cata::optional<int> blood_draw( Character *, item *, bool, const tripoint & );
cata::optional<int> boltcutters( Character *, item *, bool, const tripoint & );
cata::optional<int> break_stick( Character *, item *, bool, const tripoint & );
cata::optional<int> c4( Character *, item *, bool, const tripoint & );
cata::optional<int> cable_attach( Character *, item *, bool, const tripoint & );
cata::optional<int> call_of_tindalos( Character *, item *, bool, const tripoint & );
cata::optional<int> camera( Character *, item *, bool, const tripoint & );
cata::optional<int> can_goo( Character *, item *, bool, const tripoint & );
cata::optional<int> capture_monster_act( Character *, item *, bool, const tripoint & );
cata::optional<int> capture_monster_veh( Character *, item *, bool, const tripoint & );
cata::optional<int> carver_off( Character *, item *, bool, const tripoint & );
cata::optional<int> carver_on( Character *, item *, bool, const tripoint & );
cata::optional<int> chainsaw_off( Character *, item *, bool, const tripoint & );
cata::optional<int> chainsaw_on( Character *, item *, bool, const tripoint & );
cata::optional<int> change_eyes( Character *, item *, bool, const tripoint & );
cata::optional<int> change_skin( Character *, item *, bool, const tripoint & );
cata::optional<int> chop_logs( Character *, item *, bool, const tripoint & );
cata::optional<int> chop_tree( Character *, item *, bool, const tripoint & );
cata::optional<int> circsaw_on( Character *, item *, bool, const tripoint & );
cata::optional<int> clear_rubble( Character *, item *, bool, const tripoint & );
cata::optional<int> coin_flip( Character *, item *, bool, const tripoint & );
cata::optional<int> combatsaw_off( Character *, item *, bool, const tripoint & );
cata::optional<int> combatsaw_on( Character *, item *, bool, const tripoint & );
cata::optional<int> contacts( Character *, item *, bool, const tripoint & );
cata::optional<int> cord_attach( Character *, item *, bool, const tripoint & );
cata::optional<int> crowbar( Character *, item *, bool, const tripoint & );
cata::optional<int> crowbar_weak( Character *, item *, bool, const tripoint & );
cata::optional<int> dig( Character *, item *, bool, const tripoint & );
cata::optional<int> dig_channel( Character *, item *, bool, const tripoint & );
cata::optional<int> directional_antenna( Character *, item *, bool, const tripoint & );
cata::optional<int> directional_hologram( Character *, item *, bool, const tripoint & );
cata::optional<int> dive_tank( Character *, item *, bool, const tripoint & );
cata::optional<int> dog_whistle( Character *, item *, bool, const tripoint & );
cata::optional<int> e_combatsaw_off( Character *, item *, bool, const tripoint & );
cata::optional<int> e_combatsaw_on( Character *, item *, bool, const tripoint & );
cata::optional<int> ehandcuffs( Character *, item *, bool, const tripoint & );
cata::optional<int> einktabletpc( Character *, item *, bool, const tripoint & );
cata::optional<int> elec_chainsaw_off( Character *, item *, bool, const tripoint & );
cata::optional<int> elec_chainsaw_on( Character *, item *, bool, const tripoint & );
cata::optional<int> emf_passive_off( Character *, item *, bool, const tripoint & );
cata::optional<int> emf_passive_on( Character *, item *, bool, const tripoint & );
cata::optional<int> extinguisher( Character *, item *, bool, const tripoint & );
cata::optional<int> fill_pit( Character *, item *, bool, const tripoint & );
cata::optional<int> firecracker( Character *, item *, bool, const tripoint & );
cata::optional<int> firecracker_act( Character *, item *, bool, const tripoint & );
cata::optional<int> firecracker_pack( Character *, item *, bool, const tripoint & );
cata::optional<int> firecracker_pack_act( Character *, item *, bool, const tripoint & );
cata::optional<int> fish_trap( Character *, item *, bool, const tripoint & );
cata::optional<int> fishing_rod( Character *, item *, bool, const tripoint & );
cata::optional<int> fitness_check( Character *p, item *it, bool, const tripoint & );
cata::optional<int> foodperson( Character *, item *, bool, const tripoint & );
cata::optional<int> gasmask( Character *, item *, bool, const tripoint & );
cata::optional<int> geiger( Character *, item *, bool, const tripoint & );
cata::optional<int> granade( Character *, item *, bool, const tripoint & );
cata::optional<int> granade_act( Character *, item *, bool, const tripoint & );
cata::optional<int> grenade_inc_act( Character *, item *, bool, const tripoint & );
cata::optional<int> gun_repair( Character *, item *, bool, const tripoint & );
cata::optional<int> gunmod_attach( Character *, item *, bool, const tripoint & );
cata::optional<int> hacksaw( Character *, item *, bool, const tripoint & );
cata::optional<int> hairkit( Character *, item *, bool, const tripoint & );
cata::optional<int> hammer( Character *, item *, bool, const tripoint & );
cata::optional<int> hand_crank( Character *, item *, bool, const tripoint & );
cata::optional<int> heat_food( Character *, item *, bool, const tripoint & );
cata::optional<int> heatpack( Character *, item *, bool, const tripoint & );
cata::optional<int> hotplate( Character *, item *, bool, const tripoint & );
cata::optional<int> hotplate_atomic( Character *, item *, bool, const tripoint & );
cata::optional<int> jackhammer( Character *, item *, bool, const tripoint & );
cata::optional<int> jet_injector( Character *, item *, bool, const tripoint & );
cata::optional<int> ladder( Character *, item *it, bool, const tripoint & );
cata::optional<int> lumber( Character *, item *, bool, const tripoint & );
cata::optional<int> ma_manual( Character *, item *, bool, const tripoint & );
cata::optional<int> magic_8_ball( Character *, item *, bool, const tripoint & );
cata::optional<int> electricstorage( Character *, item *, bool, const tripoint & );
cata::optional<int> ebooksave( Character *, item *, bool, const tripoint & );
cata::optional<int> ebookread( Character *, item *, bool, const tripoint & );
cata::optional<int> makemound( Character *, item *, bool, const tripoint & );
cata::optional<int> manage_exosuit( Character *, item *, bool, const tripoint & );
cata::optional<int> melatonin_tablet( Character *, item *, bool, const tripoint & );
cata::optional<int> mininuke( Character *, item *, bool, const tripoint & );
cata::optional<int> molotov_lit( Character *, item *, bool, const tripoint & );
cata::optional<int> mop( Character *, item *, bool, const tripoint & );
cata::optional<int> mp3( Character *, item *, bool, const tripoint & );
cata::optional<int> mp3_on( Character *, item *, bool, const tripoint & );
cata::optional<int> noise_emitter_off( Character *, item *, bool, const tripoint & );
cata::optional<int> noise_emitter_on( Character *, item *, bool, const tripoint & );
cata::optional<int> oxygen_bottle( Character *, item *, bool, const tripoint & );
cata::optional<int> oxytorch( Character *, item *, bool, const tripoint & );
cata::optional<int> binder_add_recipe( Character *, item *, bool, const tripoint & );
cata::optional<int> binder_manage_recipe( Character *, item *, bool, const tripoint & );
cata::optional<int> pack_cbm( Character *p, item *it, bool, const tripoint & );
cata::optional<int> pack_item( Character *, item *, bool, const tripoint & );
cata::optional<int> pick_lock( Character *p, item *it, bool, const tripoint &pos );
cata::optional<int> pickaxe( Character *, item *, bool, const tripoint & );
cata::optional<int> play_game( Character *, item *, bool, const tripoint & );
cata::optional<int> portable_game( Character *, item *, bool active, const tripoint & );
cata::optional<int> portal( Character *, item *, bool, const tripoint & );
cata::optional<int> radglove( Character *, item *, bool, const tripoint & );
cata::optional<int> radio_mod( Character *, item *, bool, const tripoint & );
cata::optional<int> radio_off( Character *, item *, bool, const tripoint & );
cata::optional<int> radio_on( Character *, item *, bool, const tripoint & );
cata::optional<int> remove_all_mods( Character *, item *, bool, const tripoint & );
cata::optional<int> rm13armor_off( Character *, item *, bool, const tripoint & );
cata::optional<int> rm13armor_on( Character *, item *, bool, const tripoint & );
cata::optional<int> robotcontrol( Character *, item *, bool active, const tripoint & );
cata::optional<int> rpgdie( Character *, item *, bool, const tripoint & );
cata::optional<int> seed( Character *, item *, bool, const tripoint & );
cata::optional<int> shavekit( Character *, item *, bool, const tripoint & );
cata::optional<int> shocktonfa_off( Character *, item *, bool, const tripoint & );
cata::optional<int> shocktonfa_on( Character *, item *, bool, const tripoint & );
cata::optional<int> siphon( Character *, item *, bool, const tripoint & );
cata::optional<int> solarpack( Character *, item *, bool, const tripoint & );
cata::optional<int> solarpack_off( Character *, item *, bool, const tripoint & );
cata::optional<int> spray_can( Character *, item *, bool, const tripoint & );
cata::optional<int> stimpack( Character *, item *, bool, const tripoint & );
cata::optional<int> strong_antibiotic( Character *, item *, bool, const tripoint & );
cata::optional<int> talking_doll( Character *, item *, bool, const tripoint & );
cata::optional<int> tazer( Character *, item *, bool, const tripoint & );
cata::optional<int> tazer2( Character *, item *, bool, const tripoint & );
cata::optional<int> teleport( Character *, item *, bool, const tripoint & );
cata::optional<int> toolmod_attach( Character *, item *, bool, const tripoint & );
cata::optional<int> tow_attach( Character *, item *, bool, const tripoint & );
cata::optional<int> towel( Character *, item *, bool, const tripoint & );
cata::optional<int> trimmer_off( Character *, item *, bool, const tripoint & );
cata::optional<int> trimmer_on( Character *, item *, bool, const tripoint & );
cata::optional<int> unfold_generic( Character *, item *, bool, const tripoint & );
cata::optional<int> unpack_item( Character *, item *, bool, const tripoint & );
cata::optional<int> vibe( Character *, item *, bool, const tripoint & );
cata::optional<int> voltmeter( Character *p, item *it, bool, const tripoint & );
cata::optional<int> vortex( Character *, item *, bool, const tripoint & );
cata::optional<int> wash_all_items( Character *, item *, bool, const tripoint & );
cata::optional<int> wash_hard_items( Character *, item *, bool, const tripoint & );
cata::optional<int> wash_items( Character *p, bool soft_items, bool hard_items );
cata::optional<int> wash_soft_items( Character *, item *, bool, const tripoint & );
cata::optional<int> water_purifier( Character *, item *, bool, const tripoint & );
cata::optional<int> weak_antibiotic( Character *, item *, bool, const tripoint & );
cata::optional<int> weather_tool( Character *, item *, bool, const tripoint & );
cata::optional<int> sextant( Character *, item *, bool, const tripoint & );
cata::optional<int> lux_meter( Character *, item *, bool, const tripoint & );
cata::optional<int> dbg_lux_meter( Character *, item *, bool, const tripoint & );
cata::optional<int> calories_intake_tracker( Character *p, item *, bool, const tripoint & );

// MACGUFFINS

cata::optional<int> radiocar( Character *, item *, bool, const tripoint & );
cata::optional<int> radiocaron( Character *, item *, bool, const tripoint & );
cata::optional<int> radiocontrol( Character *, item *, bool, const tripoint & );

cata::optional<int> multicooker( Character *, item *, bool, const tripoint & );

cata::optional<int> remoteveh( Character *, item *, bool, const tripoint & );

cata::optional<int> craft( Character *, item *, bool, const tripoint & );

cata::optional<int> disassemble( Character *, item *, bool, const tripoint & );

// Helper functions for other iuse functions
void cut_log_into_planks( Character & );
void play_music( Character &p, const tripoint &source, int volume, int max_morale );
int towel_common( Character *, item *, bool );

// Helper for validating a potential target of robot control
bool robotcontrol_can_target( Character *, const monster & );

// Helper for handling pesky wannabe-artists
cata::optional<int> handle_ground_graffiti( Character &p, item *it, const std::string &prefix,
        const tripoint &where );

//helper for lit cigs
cata::optional<std::string> can_smoke( const Character &you );
} // namespace iuse

void remove_radio_mod( item &it, Character &p );
// used for unit testing iuse::gun_repair
cata::optional<int> gun_repair( Character *p, item *it, item_location &loc );

// Helper for clothes washing
struct washing_requirements {
    int water;
    int cleanser;
    int time;
};
washing_requirements washing_requirements_for_volume( const units::volume & );

using use_function_pointer = cata::optional<int> ( * )( Character *, item *, bool,
                             const tripoint & );

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
        virtual cata::optional<int> use( Character &, item &, bool, const tripoint & ) const = 0;
        virtual ret_val<void> can_use( const Character &, const item &, bool, const tripoint & ) const;
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
         * Returns the translated description of the action. It is used for the item action menu.
         */
        virtual std::string get_description() const;
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

        cata::optional<int> call( Character &, item &, bool, const tripoint & ) const;
        ret_val<void> can_call( const Character &, const item &, bool t, const tripoint &pos ) const;

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
        /** @return See @ref iuse_actor::get_description */
        std::string get_description() const;
        /** @return Used by @ref item::info to get description of the actor */
        void dump_info( const item &, std::vector<iteminfo> & ) const;
};

#endif // CATA_SRC_IUSE_H
