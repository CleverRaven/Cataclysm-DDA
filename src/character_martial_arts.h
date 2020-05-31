#pragma once
#ifndef CATA_SRC_CHARACTER_MARTIAL_ARTS_H
#define CATA_SRC_CHARACTER_MARTIAL_ARTS_H

#include <algorithm>
#include <string>
#include <vector>

#include "martialarts.h"
#include "type_id.h"

class Character;
class JsonIn;
class JsonOut;
class avatar;
class item;

class character_martial_arts
{
    private:
        std::vector<matype_id> ma_styles;
        matype_id style_selected = matype_id( "style_none" );
        bool keep_hands_free = false;
    public:
        character_martial_arts();
        character_martial_arts( const std::vector<matype_id> &styles,
                                const matype_id &style_selected, bool keep_hands_free )
            : ma_styles( styles ), style_selected( style_selected ), keep_hands_free( keep_hands_free ) {}

        void serialize( JsonOut &json ) const;
        void deserialize( JsonIn &jsin );

        void reset_style();
        // checks that style selected is one that is known, otherwise resets it
        void selected_style_check();
        /** Creates the UI and handles player input for picking martial arts styles */
        bool pick_style( const avatar &you );

        bool knows_selected_style() const;
        bool selected_strictly_melee() const;
        bool selected_allow_melee() const;
        bool selected_has_weapon( const itype_id &weap ) const;
        bool selected_force_unarmed() const;
        bool selected_is_none() const;

        /** Returns true if the player has access to the entered martial art */
        bool has_martialart( const matype_id &ma_id ) const;
        /** Adds the entered martial art to the player's list */
        void add_martialart( const matype_id &ma_id );
        void learn_current_style_CQB( bool is_avatar );
        void learn_style( const matype_id &mastyle, bool is_avatar );
        void set_style( const matype_id &mastyle, bool force = false );
        /** Displays a message if the player can or cannot use the martial art */
        void martialart_use_message( const Character &owner ) const;

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

        /** Returns true if the player has the leg block technique available */
        bool can_leg_block( const Character &owner ) const;
        /** Returns true if the player has the arm block technique available */
        bool can_arm_block( const Character &owner ) const;
        /** Returns true if either can_leg_block() or can_arm_block() returns true */
        bool can_limb_block( const Character &owner ) const;
        /** Returns true if the current style forces unarmed attack techniques */
        bool is_force_unarmed() const;

        std::vector<matec_id> get_all_techniques( const item &weap ) const;
        std::vector<matype_id> get_unknown_styles( const character_martial_arts &from ) const;
        /** Returns true if the player has technique-based miss recovery */
        bool has_miss_recovery_tec( const item &weap ) const;
        /** Returns the technique used for miss recovery */
        ma_technique get_miss_recovery_tec( const item &weap ) const;
        /** Returns true if the player has a grab breaking technique available */
        bool has_grab_break_tec() const;
        /** Returns true if the player has a weapon or martial arts skill available with the entered technique */
        bool has_technique( const Character &guy, const matec_id &id, const item &weap ) const;
        /** Returns the grab breaking technique if available */
        ma_technique get_grab_break_tec( const item &weap ) const;

        std::string enumerate_known_styles( const itype_id &weap ) const;
        std::string selected_style_name( const Character &owner ) const;
};

#endif // CATA_SRC_CHARACTER_MARTIAL_ARTS_H
