#pragma once
#ifndef CATA_SRC_IUSE_H
#define CATA_SRC_IUSE_H

#include <iosfwd>
#include <memory>
#include <optional>
#include <type_traits>
#include <vector>

#include "clone_ptr.h"
#include "item_location.h"
#include "type_id.h"
#include "units_fwd.h"

class Character;
class JsonObject;
class item;
class monster;
struct iteminfo;
struct tripoint;
template<typename T> class ret_val;

// iuse methods return the number of charges expended, which is usually "1", or no value.
// Returning 0 indicates the item has not been used up, though it may have been successfully activated.
// 0 may also mean that the consumption and time progress was handled within iuse action.
// If the item is destroyed here the return value must be 0.
// A return of std::nullopt means it was not used at all.
namespace iuse
{

// FOOD AND DRUGS (ADMINISTRATION)
std::optional<int> alcohol_medium( Character *, item *, bool, const tripoint & );
std::optional<int> alcohol_strong( Character *, item *, bool, const tripoint & );
std::optional<int> alcohol_weak( Character *, item *, bool, const tripoint & );
std::optional<int> antibiotic( Character *, item *, bool, const tripoint & );
std::optional<int> anticonvulsant( Character *, item *, bool, const tripoint & );
std::optional<int> antifungal( Character *, item *, bool, const tripoint & );
std::optional<int> antiparasitic( Character *, item *, bool, const tripoint & );
std::optional<int> blech( Character *, item *, bool, const tripoint & );
std::optional<int> blech_because_unclean( Character *, item *, bool, const tripoint & );
std::optional<int> chew( Character *, item *, bool, const tripoint & );
std::optional<int> coke( Character *, item *, bool, const tripoint & );
std::optional<int> datura( Character *, item *, bool, const tripoint & );
std::optional<int> ecig( Character *, item *, bool, const tripoint & );
std::optional<int> eyedrops( Character *, item *, bool, const tripoint & );
std::optional<int> flu_vaccine( Character *, item *, bool, const tripoint & );
std::optional<int> flumed( Character *, item *, bool, const tripoint & );
std::optional<int> flusleep( Character *, item *, bool, const tripoint & );
std::optional<int> fungicide( Character *, item *, bool, const tripoint & );
std::optional<int> honeycomb( Character *, item *, bool, const tripoint & );
std::optional<int> inhaler( Character *, item *, bool, const tripoint & );
std::optional<int> marloss( Character *, item *, bool, const tripoint & );
std::optional<int> marloss_gel( Character *, item *, bool, const tripoint & );
std::optional<int> marloss_seed( Character *, item *, bool, const tripoint & );
std::optional<int> meditate( Character *, item *, bool, const tripoint & );
std::optional<int> meth( Character *, item *, bool, const tripoint & );
std::optional<int> mycus( Character *, item *, bool, const tripoint & );
std::optional<int> petfood( Character *p, item *it, bool, const tripoint & );
std::optional<int> plantblech( Character *, item *, bool, const tripoint & );
std::optional<int> poison( Character *, item *, bool, const tripoint & );
std::optional<int> prozac( Character *, item *, bool, const tripoint & );
std::optional<int> purify_smart( Character *, item *, bool, const tripoint & );
std::optional<int> sewage( Character *, item *, bool, const tripoint & );
std::optional<int> smoking( Character *, item *, bool, const tripoint & );
std::optional<int> thorazine( Character *, item *, bool, const tripoint & );
std::optional<int> weed_cake( Character *, item *, bool, const tripoint & );
std::optional<int> xanax( Character *, item *, bool, const tripoint & );

// TOOLS
std::optional<int> acidbomb_act( Character *, item *, bool, const tripoint & );
std::optional<int> adrenaline_injector( Character *, item *, bool, const tripoint & );
std::optional<int> afs_translocator( Character *, item *, bool, const tripoint & );
std::optional<int> bell( Character *, item *, bool, const tripoint & );
std::optional<int> blood_draw( Character *, item *, bool, const tripoint & );
std::optional<int> boltcutters( Character *, item *, bool, const tripoint & );
std::optional<int> break_stick( Character *, item *, bool, const tripoint & );
std::optional<int> c4( Character *, item *, bool, const tripoint & );
std::optional<int> call_of_tindalos( Character *, item *, bool, const tripoint & );
std::optional<int> camera( Character *, item *, bool, const tripoint & );
std::optional<int> can_goo( Character *, item *, bool, const tripoint & );
std::optional<int> capture_monster_act( Character *, item *, bool, const tripoint & );
std::optional<int> capture_monster_veh( Character *, item *, bool, const tripoint & );
std::optional<int> carver_off( Character *, item *, bool, const tripoint & );
std::optional<int> carver_on( Character *, item *, bool, const tripoint & );
std::optional<int> chainsaw_off( Character *, item *, bool, const tripoint & );
std::optional<int> chainsaw_on( Character *, item *, bool, const tripoint & );
std::optional<int> change_eyes( Character *, item *, bool, const tripoint & );
std::optional<int> change_skin( Character *, item *, bool, const tripoint & );
std::optional<int> chop_logs( Character *, item *, bool, const tripoint & );
std::optional<int> chop_tree( Character *, item *, bool, const tripoint & );
std::optional<int> circsaw_on( Character *, item *, bool, const tripoint & );
std::optional<int> clear_rubble( Character *, item *, bool, const tripoint & );
std::optional<int> coin_flip( Character *, item *, bool, const tripoint & );
std::optional<int> combatsaw_off( Character *, item *, bool, const tripoint & );
std::optional<int> combatsaw_on( Character *, item *, bool, const tripoint & );
std::optional<int> contacts( Character *, item *, bool, const tripoint & );
std::optional<int> crowbar( Character *, item *, bool, const tripoint & );
std::optional<int> crowbar_weak( Character *, item *, bool, const tripoint & );
std::optional<int> dig( Character *, item *, bool, const tripoint & );
std::optional<int> dig_channel( Character *, item *, bool, const tripoint & );
std::optional<int> directional_antenna( Character *, item *, bool, const tripoint & );
std::optional<int> directional_hologram( Character *, item *, bool, const tripoint & );
std::optional<int> dive_tank( Character *, item *, bool, const tripoint & );
std::optional<int> dog_whistle( Character *, item *, bool, const tripoint & );
std::optional<int> e_combatsaw_off( Character *, item *, bool, const tripoint & );
std::optional<int> e_combatsaw_on( Character *, item *, bool, const tripoint & );
std::optional<int> ehandcuffs( Character *, item *, bool, const tripoint & );
std::optional<int> einktabletpc( Character *, item *, bool, const tripoint & );
std::optional<int> elec_chainsaw_off( Character *, item *, bool, const tripoint & );
std::optional<int> elec_chainsaw_on( Character *, item *, bool, const tripoint & );
std::optional<int> emf_passive_off( Character *, item *, bool, const tripoint & );
std::optional<int> emf_passive_on( Character *, item *, bool, const tripoint & );
std::optional<int> extinguisher( Character *, item *, bool, const tripoint & );
std::optional<int> fill_pit( Character *, item *, bool, const tripoint & );
std::optional<int> firecracker( Character *, item *, bool, const tripoint & );
std::optional<int> firecracker_act( Character *, item *, bool, const tripoint & );
std::optional<int> firecracker_pack( Character *, item *, bool, const tripoint & );
std::optional<int> firecracker_pack_act( Character *, item *, bool, const tripoint & );
std::optional<int> fish_trap( Character *, item *, bool, const tripoint & );
std::optional<int> fishing_rod( Character *, item *, bool, const tripoint & );
std::optional<int> fitness_check( Character *p, item *it, bool, const tripoint & );
std::optional<int> foodperson( Character *, item *, bool, const tripoint & );
std::optional<int> gasmask( Character *, item *, bool, const tripoint & );
std::optional<int> geiger( Character *, item *, bool, const tripoint & );
std::optional<int> granade( Character *, item *, bool, const tripoint & );
std::optional<int> granade_act( Character *, item *, bool, const tripoint & );
std::optional<int> grenade_inc_act( Character *, item *, bool, const tripoint & );
std::optional<int> gun_repair( Character *, item *, bool, const tripoint & );
std::optional<int> gunmod_attach( Character *, item *, bool, const tripoint & );
std::optional<int> hacksaw( Character *, item *, bool, const tripoint & );
std::optional<int> hairkit( Character *, item *, bool, const tripoint & );
std::optional<int> hammer( Character *, item *, bool, const tripoint & );
std::optional<int> hand_crank( Character *, item *, bool, const tripoint & );
std::optional<int> heat_food( Character *, item *, bool, const tripoint & );
std::optional<int> heatpack( Character *, item *, bool, const tripoint & );
std::optional<int> hotplate( Character *, item *, bool, const tripoint & );
std::optional<int> hotplate_atomic( Character *, item *, bool, const tripoint & );
std::optional<int> jackhammer( Character *, item *, bool, const tripoint & );
std::optional<int> jet_injector( Character *, item *, bool, const tripoint & );
std::optional<int> lumber( Character *, item *, bool, const tripoint & );
std::optional<int> ma_manual( Character *, item *, bool, const tripoint & );
std::optional<int> magic_8_ball( Character *, item *, bool, const tripoint & );
std::optional<int> electricstorage( Character *, item *, bool, const tripoint & );
std::optional<int> ebooksave( Character *, item *, bool, const tripoint & );
std::optional<int> ebookread( Character *, item *, bool, const tripoint & );
std::optional<int> makemound( Character *, item *, bool, const tripoint & );
std::optional<int> manage_exosuit( Character *, item *, bool, const tripoint & );
std::optional<int> melatonin_tablet( Character *, item *, bool, const tripoint & );
std::optional<int> mininuke( Character *, item *, bool, const tripoint & );
std::optional<int> molotov_lit( Character *, item *, bool, const tripoint & );
std::optional<int> mop( Character *, item *, bool, const tripoint & );
std::optional<int> mp3( Character *, item *, bool, const tripoint & );
std::optional<int> mp3_on( Character *, item *, bool, const tripoint & );
std::optional<int> noise_emitter_off( Character *, item *, bool, const tripoint & );
std::optional<int> noise_emitter_on( Character *, item *, bool, const tripoint & );
std::optional<int> oxygen_bottle( Character *, item *, bool, const tripoint & );
std::optional<int> oxytorch( Character *, item *, bool, const tripoint & );
std::optional<int> binder_add_recipe( Character *, item *, bool, const tripoint & );
std::optional<int> binder_manage_recipe( Character *, item *, bool, const tripoint & );
std::optional<int> pack_cbm( Character *p, item *it, bool, const tripoint & );
std::optional<int> pack_item( Character *, item *, bool, const tripoint & );
std::optional<int> pick_lock( Character *p, item *it, bool, const tripoint &pos );
std::optional<int> pickaxe( Character *, item *, bool, const tripoint & );
std::optional<int> play_game( Character *, item *, bool, const tripoint & );
std::optional<int> plug_in( Character *, item *, bool, const tripoint & );
std::optional<int> portable_game( Character *, item *, bool active, const tripoint & );
std::optional<int> portal( Character *, item *, bool, const tripoint & );
std::optional<int> radglove( Character *, item *, bool, const tripoint & );
std::optional<int> radio_mod( Character *, item *, bool, const tripoint & );
std::optional<int> radio_off( Character *, item *, bool, const tripoint & );
std::optional<int> radio_on( Character *, item *, bool, const tripoint & );
std::optional<int> remove_all_mods( Character *, item *, bool, const tripoint & );
std::optional<int> rm13armor_off( Character *, item *, bool, const tripoint & );
std::optional<int> rm13armor_on( Character *, item *, bool, const tripoint & );
std::optional<int> robotcontrol( Character *, item *, bool active, const tripoint & );
std::optional<int> rpgdie( Character *, item *, bool, const tripoint & );
std::optional<int> seed( Character *, item *, bool, const tripoint & );
std::optional<int> shavekit( Character *, item *, bool, const tripoint & );
std::optional<int> shocktonfa_off( Character *, item *, bool, const tripoint & );
std::optional<int> shocktonfa_on( Character *, item *, bool, const tripoint & );
std::optional<int> siphon( Character *, item *, bool, const tripoint & );
std::optional<int> solarpack( Character *, item *, bool, const tripoint & );
std::optional<int> solarpack_off( Character *, item *, bool, const tripoint & );
std::optional<int> spray_can( Character *, item *, bool, const tripoint & );
std::optional<int> stimpack( Character *, item *, bool, const tripoint & );
std::optional<int> strong_antibiotic( Character *, item *, bool, const tripoint & );
std::optional<int> talking_doll( Character *, item *, bool, const tripoint & );
std::optional<int> tazer( Character *, item *, bool, const tripoint & );
std::optional<int> tazer2( Character *, item *, bool, const tripoint & );
std::optional<int> teleport( Character *, item *, bool, const tripoint & );
std::optional<int> toolmod_attach( Character *, item *, bool, const tripoint & );
std::optional<int> towel( Character *, item *, bool, const tripoint & );
std::optional<int> trimmer_off( Character *, item *, bool, const tripoint & );
std::optional<int> trimmer_on( Character *, item *, bool, const tripoint & );
std::optional<int> unfold_generic( Character *, item *, bool, const tripoint & );
std::optional<int> unpack_item( Character *, item *, bool, const tripoint & );
std::optional<int> vibe( Character *, item *, bool, const tripoint & );
std::optional<int> voltmeter( Character *p, item *it, bool, const tripoint & );
std::optional<int> vortex( Character *, item *, bool, const tripoint & );
std::optional<int> wash_all_items( Character *, item *, bool, const tripoint & );
std::optional<int> wash_hard_items( Character *, item *, bool, const tripoint & );
std::optional<int> wash_items( Character *p, bool soft_items, bool hard_items );
std::optional<int> wash_soft_items( Character *, item *, bool, const tripoint & );
std::optional<int> water_purifier( Character *, item *, bool, const tripoint & );
std::optional<int> weak_antibiotic( Character *, item *, bool, const tripoint & );
std::optional<int> weather_tool( Character *, item *, bool, const tripoint & );
std::optional<int> sextant( Character *, item *, bool, const tripoint & );
std::optional<int> lux_meter( Character *, item *, bool, const tripoint & );
std::optional<int> dbg_lux_meter( Character *, item *, bool, const tripoint & );
std::optional<int> calories_intake_tracker( Character *p, item *, bool, const tripoint & );

// MACGUFFINS

std::optional<int> radiocar( Character *, item *, bool, const tripoint & );
std::optional<int> radiocaron( Character *, item *, bool, const tripoint & );
std::optional<int> radiocontrol( Character *, item *, bool, const tripoint & );

std::optional<int> multicooker( Character *, item *, bool, const tripoint & );

std::optional<int> remoteveh( Character *, item *, bool, const tripoint & );

std::optional<int> craft( Character *, item *, bool, const tripoint & );

std::optional<int> disassemble( Character *, item *, bool, const tripoint & );

// Helper functions for other iuse functions
void cut_log_into_planks( Character & );
void play_music( Character &p, const tripoint &source, int volume, int max_morale );
int towel_common( Character *, item *, bool );

// Helper for validating a potential target of robot control
bool robotcontrol_can_target( Character *, const monster & );

// Helper for handling pesky wannabe-artists
std::optional<int> handle_ground_graffiti( Character &p, item *it, const std::string &prefix,
        const tripoint &where );

//helper for lit cigs
std::optional<std::string> can_smoke( const Character &you );
} // namespace iuse

void remove_radio_mod( item &it, Character &p );
// used for unit testing iuse::gun_repair
std::optional<int> gun_repair( Character *p, item *it, item_location &loc );

// Helper for clothes washing
struct washing_requirements {
    int water;
    int cleanser;
    int time;
};
washing_requirements washing_requirements_for_volume( const units::volume & );

using use_function_pointer = std::optional<int> ( * )( Character *, item *, bool,
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
        virtual std::optional<int> use( Character &, item &, bool, const tripoint & ) const = 0;
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

        std::optional<int> call( Character &, item &, bool, const tripoint & ) const;
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
