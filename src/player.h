#pragma once
#ifndef CATA_SRC_PLAYER_H
#define CATA_SRC_PLAYER_H

#include <iosfwd>
#include <list>
#include <map>
#include <set>
#include <type_traits>
#include <utility>
#include <vector>

#include "character.h"
#include "character_id.h"
#include "creature.h"
#include "enums.h"
#include "game_constants.h"
#include "item.h"
#include "item_location.h"
#include "item_pocket.h"
#include "memory_fast.h"
#include "optional.h"
#include "point.h"
#include "ret_val.h"
#include "type_id.h"

class craft_command;
class JsonIn;
class JsonObject;
class JsonOut;
class nc_color;
class profession;
class time_duration;
class time_point;
class vehicle;

namespace catacurses
{
class window;
}  // namespace catacurses
struct damage_unit;
struct dealt_projectile_attack;
struct trap;

enum action_id : int;
enum game_message_type : int;
enum class recipe_filter_flags : int;

nc_color encumb_color( int level );

/** @relates ret_val */
template<>
struct ret_val<edible_rating>::default_success : public
    std::integral_constant<edible_rating, EDIBLE> {};

/** @relates ret_val */
template<>
struct ret_val<edible_rating>::default_failure : public
    std::integral_constant<edible_rating, INEDIBLE> {};

struct needs_rates {
    float thirst = 0.0f;
    float hunger = 0.0f;
    float fatigue = 0.0f;
    float recovery = 0.0f;
    float kcal = 0.0f;
};

class player : public Character
{
    public:
        player();
        player( const player & ) = delete;
        player( player && ) noexcept( map_is_noexcept );
        ~player() override;
        player &operator=( const player & ) = delete;
        player &operator=( player && ) noexcept( list_is_noexcept );

        /** Calls Character::normalize()
         *  normalizes HP and body temperature
         */

        void normalize() override;

        player *as_player() override {
            return this;
        }
        const player *as_player() const override {
            return this;
        }

        bool is_npc() const override {
            return false;    // Overloaded for NPCs in npc.h
        }

        // populate variables, inventory items, and misc from json object
        virtual void deserialize( JsonIn &jsin ) = 0;

        // by default save all contained info
        virtual void serialize( JsonOut &jsout ) const = 0;

        /** Handles and displays detailed character info for the '@' screen */
        void disp_info();

        /** Resets movement points and applies other non-idempotent changes */
        void process_turn() override;

        void pause(); // '.' command; pauses & resets recoil

        /**
         * Checks both the neighborhoods of from and to for climbable surfaces,
         * returns move cost of climbing from `from` to `to`.
         * 0 means climbing is not possible.
         * Return value can depend on the orientation of the terrain.
         */
        int climbing_cost( const tripoint &from, const tripoint &to ) const;

        /** Used for eating object at a location. Removes item if all of it was consumed.
        *   @returns trinary enum NONE, SOME or ALL amount consumed.
        */
        trinary consume( item_location loc, bool force = false, bool refuel = false );

        /** Used for eating a particular item that doesn't need to be in inventory.
         *  @returns trinary enum NONE, SOME or ALL (doesn't remove).
         */
        trinary consume( item &target, bool force = false, bool refuel = false );

        /** Check player strong enough to lift an object unaided by equipment (jacks, levers etc) */
        template <typename T> bool can_lift( const T &obj ) const;

        /**
         * Attempt to mend an item (fix any current faults)
         * @param obj Object to mend
         * @param interactive if true prompts player when multiple faults, otherwise mends the first
         */
        void mend_item( item_location &&obj, bool interactive = true );

        /** Wear item; returns false on fail. If interactive is false, don't alert the player or drain moves on completion. */
        cata::optional<std::list<item>::iterator>
        wear( int pos, bool interactive = true );

        /** Wear item; returns false on fail. If interactive is false, don't alert the player or drain moves on completion.
        * @param item_wear item_location of item to be worn.
        * @param interactive Alert player and drain moves if true.
        */
        cata::optional<std::list<item>::iterator>
        wear( item_location item_wear, bool interactive = true );

        /**
         * Try to wield a contained item consuming moves proportional to weapon skill and volume.
         * @param container Container containing the item to be wielded
         * @param internal_item reference to contained item to wield.
         * @param penalties Whether item volume and temporary effects (e.g. GRABBED, DOWNED) should be considered.
         * @param base_cost Cost due to storage type.
         */
        bool wield_contents( item &container, item *internal_item = nullptr, bool penalties = true,
                             int base_cost = INVENTORY_HANDLING_PENALTY );
        /**
         * Stores an item inside another consuming moves proportional to weapon skill and volume
         * @param container Container in which to store the item
         * @param put Item to add to the container
         * @param penalties Whether item volume and temporary effects (e.g. GRABBED, DOWNED) should be considered.
         * @param base_cost Cost due to storage type.
         */
        void store( item &container, item &put, bool penalties = true,
                    int base_cost = INVENTORY_HANDLING_PENALTY,
                    item_pocket::pocket_type pk_type = item_pocket::pocket_type::CONTAINER );
        /** Draws the UI and handles player input for the armor re-ordering window */
        void sort_armor();
        /** Uses a tool */
        void use( int inventory_position );
        /** Uses a tool at location */
        void use( item_location loc, int pre_obtain_moves = -1 );
        /** Uses the current wielded weapon */
        void use_wielded();

        /**
         * Starts activity to remove gunmod after unloading any contained ammo.
         * Returns true on success (activity has been started)
         */
        bool gunmod_remove( item &gun, item &mod );

        /** Starts activity to install gunmod having warned user about any risk of failure or irremovable mods s*/
        void gunmod_add( item &gun, item &mod );

        /** Starts activity to install toolmod */
        void toolmod_add( item_location tool, item_location mod );

        /** Handles sleep attempts by the player, starts ACT_TRY_SLEEP activity */
        void try_to_sleep( const time_duration &dur );

        // ---------------VALUES-----------------
        // Relative direction of a grab, add to posx, posy to get the coordinates of the grabbed thing.
        tripoint grab_point;
        int volume = 0;
        const profession *prof;
        std::set<const profession *> hobbies;

        bool random_start_location = true;
        start_location_id start_location;

        weak_ptr_fast<Creature> last_target;
        cata::optional<tripoint> last_target_pos;
        // Save favorite ammo location
        item_location ammo_location;
        int scent = 0;
        int cash = 0;

        bool manual_examine = false;

        std::set<character_id> follower_ids;
        void mod_stat( const std::string &stat, float modifier ) override;

        void environmental_revert_effect();

        //message related stuff
        using Character::add_msg_if_player;
        void add_msg_if_player( const std::string &msg ) const override;
        void add_msg_if_player( const game_message_params &params, const std::string &msg ) const override;
        using Character::add_msg_debug_if_player;
        void add_msg_debug_if_player( debugmode::debug_filter type,
                                      const std::string &msg ) const override;
        using Character::add_msg_player_or_npc;
        void add_msg_player_or_npc( const std::string &player_msg,
                                    const std::string &npc_str ) const override;
        void add_msg_player_or_npc( const game_message_params &params, const std::string &player_msg,
                                    const std::string &npc_msg ) const override;
        using Character::add_msg_debug_player_or_npc;
        void add_msg_debug_player_or_npc( debugmode::debug_filter type, const std::string &player_msg,
                                          const std::string &npc_msg ) const override;
        using Character::add_msg_player_or_say;
        void add_msg_player_or_say( const std::string &player_msg,
                                    const std::string &npc_speech ) const override;
        void add_msg_player_or_say( const game_message_params &params, const std::string &player_msg,
                                    const std::string &npc_speech ) const override;

        // formats and prints encumbrance info to specified window
        void print_encumbrance( const catacurses::window &win, int line = -1,
                                const item *selected_clothing = nullptr ) const;

        using Character::query_yn;
        bool query_yn( const std::string &mes ) const override;

    protected:

        void store( JsonOut &json ) const;
        void load( const JsonObject &data );

    private:

        /** warnings from a faction about bad behavior */
        std::map<faction_id, std::pair<int, time_point>> warning_record;

};

extern template bool player::can_lift<item>( const item &obj ) const;
extern template bool player::can_lift<vehicle>( const vehicle &obj ) const;

#endif // CATA_SRC_PLAYER_H
