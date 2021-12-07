#pragma once
#ifndef CATA_SRC_CHARACTER_MODIFIER_H
#define CATA_SRC_CHARACTER_MODIFIER_H

#include <vector>

#include "bodypart.h"
#include "json.h"
#include "translation.h"
#include "type_id.h"

struct character_modifier {
    public:
        enum mod_type {
            NONE,
            ADD,
            MULT
        };

        static void load_character_modifiers( const JsonObject &jo, const std::string &src );
        static void reset();
        void load( const JsonObject &jo, const std::string &src );
        static const std::vector<character_modifier> &get_all();

        // Use this to obtain the calculated modifier
        float modifier( const Character &c, const skill_id &skill = skill_id::NULL_ID() ) const;

        const character_modifier_id &getId() const {
            return id;
        }
        // Description of the modifier to display in the UI
        const translation &description() const {
            return desc;
        }
        // Whether the modifier is added or multiplied
        const mod_type &modifier_type() const {
            return modtype;
        }
        // Same as above, but for displaying in the UI
        std::string mod_type_str() const {
            switch( modtype ) {
                case ADD:
                    return "+";
                case MULT:
                    return "x";
                case NONE:
                default:
                    break;
            }
            return std::string();
        }
        // Does this modifier use a built-in function to calculate the modifier?
        bool is_builtin() const {
            return !builtin.empty();
        }
        // Which limb score is used to calculate this modifier (if any)
        const limb_score_id &use_limb_score() const {
            return limbscore;
        }

    private:
        character_modifier_id id = character_modifier_id::NULL_ID();
        limb_score_id limbscore = limb_score_id::NULL_ID();
        body_part_type::type limbtype = body_part_type::type::num_types;
        translation desc = translation();
        mod_type modtype = mod_type::NONE;
        float max_val = 0.0f;
        float min_val = 0.0f;
        float nominator = 0.0f;
        float subtractor = 0.0f;
        int override_encumb = -1;
        int override_wounds = -1;
        std::string builtin;
        bool was_loaded = false;
        friend class generic_factory<character_modifier>;
};

#endif // CATA_SRC_CHARACTER_MODIFIER_H