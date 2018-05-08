#pragma once
#ifndef BODYPART_H
#define BODYPART_H

#include <string>
#include <array>
#include <bitset>

#include "string_id.h"
#include "int_id.h"

class JsonObject;

// The order is important ; pldata.h has to be in the same order
enum body_part : int {
    bp_torso = 0,
    bp_head,
    bp_eyes,
    bp_mouth,
    bp_arm_l,
    bp_arm_r,
    bp_hand_l,
    bp_hand_r,
    bp_leg_l,
    bp_leg_r,
    bp_foot_l,
    bp_foot_r,
    num_bp
};

enum class side : int {
    BOTH,
    LEFT,
    RIGHT
};

/**
 * Contains all valid @ref body_part values in the order they are
 * defined in. Use this to iterate over them.
 */
const constexpr std::array<body_part, 12> all_body_parts = {{
        bp_torso, bp_head, bp_eyes, bp_mouth,
        bp_arm_l, bp_arm_r, bp_hand_l, bp_hand_r,
        bp_leg_l, bp_leg_r, bp_foot_l, bp_foot_r
    }
};

struct body_part_struct;
using bodypart_ids = string_id<body_part_struct>;
using bodypart_id = int_id<body_part_struct>;

struct body_part_struct {
    public:
        bodypart_ids id;
        bool was_loaded = false;

        // Those are stored untranslated
        std::string name;
        std::string name_as_heading_singular;
        std::string name_as_heading_multiple;
        std::string hp_bar_ui_text;
        std::string encumb_text;
        // Legacy "string id"
        std::string legacy_id = "num_bp";
        // Legacy enum "int id"
        body_part token = num_bp;
        /** Size of the body part when doing an unweighted selection. */
        float hit_size = 0.0f;
        /** Hit sizes for attackers who are smaller, equal in size, and bigger. */
        std::array<float, 3> hit_size_relative = {{ 0.0f, 0.0f, 0.0f }};
        /**
         * How hard is it to hit a given body part, assuming "owner" is hit.
         * Higher number means good hits will veer towards this part,
         * lower means this part is unlikely to be hit by inaccurate attacks.
         * Formula is `chance *= pow(hit_roll, hit_difficulty)`
         */
        float hit_difficulty = 0.0f;
        // "Parent" of this part - main parts are their own "parents"
        // @todo: Connect head and limbs to torso
        bodypart_ids main_part;
        // A part that has no opposite is its own opposite (that's pretty Zen)
        bodypart_ids opposite_part;
        // Parts with no opposites have BOTH here
        side part_side = side::BOTH;

        void load( JsonObject &jo, const std::string &src );
        void finalize();
        void check() const;

        static void load_bp( JsonObject &jo, const std::string &src );

        // Clears all bps
        static void reset();
        // Post-load finalization
        static void finalize_all();
        // Verifies that body parts make sense
        static void check_consistency();
};

class body_part_set
{
    private:
        std::bitset<num_bp> parts;

        explicit body_part_set( const std::bitset<num_bp> &other ) : parts( other ) { }

    public:
        body_part_set() = default;
        body_part_set( std::initializer_list<body_part> bps ) {
            for( const auto &bp : bps ) {
                set( bp );
            }
        }

        body_part_set &operator|=( const body_part_set &rhs ) {
            parts |= rhs.parts;
            return *this;
        }
        body_part_set &operator&=( const body_part_set &rhs ) {
            parts &= rhs.parts;
            return *this;
        }

        body_part_set operator|( const body_part_set &rhs ) const {
            return body_part_set( parts | rhs.parts );
        }
        body_part_set operator&( const body_part_set &rhs ) const {
            return body_part_set( parts & rhs.parts );
        }

        body_part_set operator~() const {
            return body_part_set( ~parts );
        }

        static body_part_set all() {
            return ~body_part_set();
        }

        bool test( const body_part &bp ) const {
            return parts.test( bp );
        }
        void set( const body_part &bp ) {
            parts.set( bp );
        }
        void reset( const body_part &bp ) {
            parts.reset( bp );
        }
        bool any() const {
            return parts.any();
        }
        bool none() const {
            return parts.none();
        }
        size_t count() const {
            return parts.count();
        }

        template<typename Stream>
        void serialize( Stream &s ) const {
            s.write( parts );
        }
        template<typename Stream>
        void deserialize( Stream &s ) {
            s.read( parts );
        }
};

/** Returns the new id for old token */
const bodypart_ids &convert_bp( body_part bp );

/** Returns the opposite side. */
side opposite_side( side s );

// identify the index of a body part's "other half", or itself if not
const std::array<size_t, 12> bp_aiOther = {{0, 1, 2, 3, 5, 4, 7, 6, 9, 8, 11, 10}};

/** Returns the matching name of the body_part token. */
std::string body_part_name( body_part bp );

/** Returns the matching accusative name of the body_part token, i.e. "Shrapnel hits your X".
 *  These are identical to body_part_name above in English, but not in some other languages. */
std::string body_part_name_accusative( body_part bp );

/** Returns the name of the body parts in a context where the name is used as
 * a heading or title e.g. "Left Arm". */
std::string body_part_name_as_heading( body_part bp, int number );

/** Returns the body part text to be displayed in the HP bar */
std::string body_part_hp_bar_ui_text( body_part bp );

/** Returns the matching encumbrance text for a given body_part token. */
std::string encumb_text( body_part bp );

/** Returns a random body_part token. main_parts_only will limit it to arms, legs, torso, and head. */
body_part random_body_part( bool main_parts_only = false );

/** Returns the matching main body_part that corresponds to the input; i.e. returns bp_arm_l from bp_hand_l. */
body_part mutate_to_main_part( body_part bp );
/** Returns the opposite body part (limb on the other side) */
body_part opposite_body_part( body_part bp );

/** Returns the matching body_part key from the corresponding body_part token. */
std::string get_body_part_id( body_part bp );

/** Returns the matching body_part token from the corresponding body_part string. */
body_part get_body_part_token( const std::string &id );


#endif
