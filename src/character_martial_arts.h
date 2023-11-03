#pragma once
#ifndef CATA_SRC_CHARACTER_MARTIAL_ARTS_H
#define CATA_SRC_CHARACTER_MARTIAL_ARTS_H

#include <iosfwd>
#include <vector>

#include "martialarts.h"
#include "type_id.h"

class Character;
class JsonObject;
class JsonOut;
class avatar;
class item;

class character_martial_arts
{
    private:
        std::vector<matype_id> ma_styles;
        matype_id style_selected = matype_id( "style_none" );
    public:
        character_martial_arts();
        character_martial_arts( const std::vector<matype_id> &styles,
                                const matype_id &style_selected, bool keep_hands_free )
            : ma_styles( styles ), style_selected( style_selected ), keep_hands_free( keep_hands_free ) {}

        void serialize( JsonOut &json ) const;
        void deserialize( const JsonObject &data );

        void reset_style();
        void clear_style( const matype_id &id );
        // checks that style selected is one that is known, otherwise resets it
        void selected_style_check();
        bool keep_hands_free = false;
        /** Creates the UI and handles player input for picking martial arts styles */
        bool pick_style( const Character &you );

        bool knows_selected_style() const;
        bool selected_strictly_melee() const;
        bool selected_allow_all_weapons() const;
        bool selected_has_weapon( const itype_id &weap ) const;
        bool selected_force_unarmed() const;
        bool selected_prevent_weapon_blocking() const;
        bool selected_is_none() const;

        /** Returns true if the player has access to the entered martial art */
        bool has_martialart( const matype_id &ma_id ) const;
        /** Adds the entered martial art to the player's list */
        void add_martialart( const matype_id &ma_id );
        void learn_current_style_CQB( bool is_avatar );
        void learn_style( const matype_id &mastyle, bool is_avatar );
        void set_style( const matype_id &mastyle, bool force = false );
        void clear_styles();
        /** Displays a message if the player can or cannot use the martial art */
        void martialart_use_message( const Character &owner ) const;

        /** Removes all martial arts events */
        void clear_all_effects( Character &owner );
        /** Fires all non-triggered martial arts events */
        void ma_static_effects( Character &owner );
        /** Fires all move-triggered martial arts events */
        void ma_onmove_effects( Character &owner );
        /** Fires all pause-triggered martial arts events */
        void ma_onpause_effects( Character &owner );
        /** Fires all hit-triggered martial arts events */
        void ma_onhit_effects( Character &owner );
        /** Fires all attack-triggered martial arts events */
        void ma_onattack_effects( Character &owner );
        /** Fires all dodge-triggered martial arts events */
        void ma_ondodge_effects( Character &owner );
        /** Fires all block-triggered martial arts events */
        void ma_onblock_effects( Character &owner );
        /** Fires all get hit-triggered martial arts events */
        void ma_ongethit_effects( Character &owner );
        /** Fires all miss-triggered martial arts events */
        void ma_onmiss_effects( Character &owner );
        /** Fires all crit-triggered martial arts events */
        void ma_oncrit_effects( Character &owner );
        /** Fires all kill-triggered martial arts events */
        void ma_onkill_effects( Character &owner );

        /** Returns an attack vector that the player can use */
        std::string get_valid_attack_vector( const Character &user,
                                             const std::vector<std::string> &attack_vectors ) const;
        /** Returns true if the player is able to use the given attack vector */
        bool can_use_attack_vector( const Character &user, const std::string &av ) const;
        /** Returns true if the player has the leg block technique available */
        bool can_leg_block( const Character &owner ) const;
        /** Returns true if the player has the arm block technique available */
        bool can_arm_block( const Character &owner ) const;
        /** Returns true if you can block with nonstandard limbs */
        bool can_nonstandard_block( const Character &owner ) const;
        /** Returns true if the current style forces unarmed attack techniques */
        bool is_force_unarmed() const;
        /** Returns true if the current style allows blocking with weapons */
        bool can_weapon_block() const;

        std::vector<matec_id> get_all_techniques( const item_location &weap, const Character &u ) const;
        std::vector<matype_id> get_unknown_styles( const character_martial_arts &from,
                bool teachable_only ) const;
        std::vector<matype_id> get_known_styles( bool teachable_only ) const;
        /** Returns true if the player has a weapon or martial arts skill available with the entered technique */
        bool has_technique( const Character &guy, const matec_id &id, const item &weap ) const;
        /** Returns the first valid grab break technique */
        ma_technique get_grab_break( const Character &owner ) const;
        /** Returns the first valid miss recovery technique */
        ma_technique get_miss_recovery( const Character &owner ) const;

        std::string enumerate_known_styles( const itype_id &weap ) const;
        std::string selected_style_name( const Character &owner ) const;
        const matype_id &selected_style() const {
            return style_selected;
        }
};

#endif // CATA_SRC_CHARACTER_MARTIAL_ARTS_H
