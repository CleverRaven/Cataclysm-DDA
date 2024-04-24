#pragma once
#ifndef CATA_SRC_SKILL_H
#define CATA_SRC_SKILL_H

#include <functional>
#include <iosfwd>
#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "calendar.h"
#include "game_constants.h"
#include "translations.h"
#include "type_id.h"

class JsonObject;
class JsonOut;
class item;
class recipe;

struct time_info_t {
    // Absolute floor on the time taken to attack.
    int min_time = 50;
    // The base or max time taken to attack.
    int base_time = 220;
    // The reduction in time given per skill level.
    int time_reduction_per_level = 25;
};

class Skill
{
        friend class string_id<Skill>;
        skill_id _ident;

        translation _name;
        translation _description;
        std::set<std::string> _tags;
        time_info_t _time_to_attack;
        skill_displayType_id _display_type;
        int _sort_rank;
        std::unordered_map<std::string, int> _companion_skill_practice;
        // these are not real skills, they depend on context
        static std::map<skill_id, Skill> contextual_skills;
        int _companion_combat_rank_factor = 0;
        int _companion_survival_rank_factor = 0;
        int _companion_industry_rank_factor = 0;
        bool _teachable = true;
        bool _obsolete = false;
    public:
        static std::vector<Skill> skills;
        static void load_skill( const JsonObject &jsobj );
        // For loading old saves that still have integer-based ids.
        static skill_id from_legacy_int( int legacy_id );
        static skill_id random_skill();

        // clear skill vector, every skill pointer becomes invalid!
        static void reset();

        static std::vector<const Skill *> get_skills_sorted_by(
            std::function<bool ( const Skill &, const Skill & )> pred );

        Skill();
        Skill( const skill_id &ident, const translation &name, const translation &description,
               const std::set<std::string> &tags, skill_displayType_id display_type );

        const skill_id &ident() const {
            return _ident;
        }
        std::string name() const {
            return _name.translated();
        }
        std::string description() const {
            return _description.translated();
        }
        int get_companion_skill_practice( const std::string &companion_skill ) const {
            return _companion_skill_practice.find( companion_skill ) == _companion_skill_practice.end() ? 0 :
                   _companion_skill_practice.at( companion_skill );
        }
        skill_displayType_id display_category() const {
            return _display_type;
        }
        int get_sort_rank() const {
            return _sort_rank;
        }
        time_info_t time_to_attack() const {
            return _time_to_attack;
        }
        int companion_combat_rank_factor() const {
            return _companion_combat_rank_factor;
        }
        int companion_survival_rank_factor() const {
            return _companion_survival_rank_factor;
        }
        int companion_industry_rank_factor() const {
            return _companion_industry_rank_factor;
        }
        bool is_teachable() const {
            return _teachable;
        }

        bool operator==( const Skill &b ) const {
            return this->_ident == b._ident;
        }
        bool operator< ( const Skill &b ) const {
            return this->_ident < b._ident;    // Only here for the benefit of std::map<Skill,T>
        }

        bool operator!=( const Skill &b ) const {
            return !( *this == b );
        }

        bool obsolete() const {
            return _obsolete;
        }

        bool is_combat_skill() const;
        bool is_contextual_skill() const;
};

class SkillLevel
{
        int _level = 0;
        int _exercise = 0;
        time_point _lastPracticed = calendar::turn;
        bool _isTraining = true;
        int _knowledgeLevel = 0;
        int _knowledgeExperience = 0;
        int _rustAccumulator = 0;

    public:
        SkillLevel() = default;

        bool isTraining() const {
            return _isTraining;
        }
        bool toggleTraining() {
            _isTraining = !_isTraining;
            return _isTraining;
        }

        int level() const {
            return std::min( _level, MAX_SKILL );
        }
        int level( int plevel ) {
            _level = plevel;
            if( _level > _knowledgeLevel ) {
                _knowledgeLevel = _level;
            }
            return level();
        }

        int knowledgeLevel() const {
            return std::min( _knowledgeLevel, MAX_SKILL );
        }
        int knowledgeLevel( int plevel ) {
            _knowledgeLevel = plevel;
            return knowledgeLevel();
        }

        int knowledgeExperience( bool raw = false ) const {
            return raw ? _knowledgeExperience : _knowledgeExperience / ( 100 * ( _knowledgeLevel + 1 ) *
                    ( _knowledgeLevel + 1 ) );
        }

        int rustAccumulator() const {
            return _rustAccumulator;
        }
        int exercise( bool raw = false ) const {
            return raw ? _exercise : _exercise / ( 100 * ( level() + 1 ) * ( level() + 1 ) );
        }

        int exercised_level() const {
            return level() * level() * 100 + exercise();
        }

        void train( int amount, float catchup_modifier, float knowledge_modifier,
                    bool allow_multilevel = false );
        void knowledge_train( int amount, int npc_knowledge = 0 );
        bool isRusty() const;
        bool rust( int rust_resist, float rust_multiplier = 1 );
        void practice();
        bool can_train() const;
        void set_exercise( int value, bool raw = false );

        void readBook( int minimumGain, int maximumGain, int maximumLevel = -1 );

        bool operator==( const SkillLevel &b ) const {
            return this->level() == b.level() && this->_exercise == b._exercise;
        }
        bool operator< ( const SkillLevel &b ) const {
            return this->level() < b.level() || ( this->level() == b.level() && this->_exercise < b._exercise );
        }
        bool operator> ( const SkillLevel &b ) const {
            return this->level() > b.level() || ( this->level() == b.level() && this->_exercise > b._exercise );
        }

        bool operator==( const int &b ) const {
            return this->level() == b;
        }
        bool operator< ( const int &b ) const {
            return this->level() < b;
        }
        bool operator> ( const int &b ) const {
            return this->level() > b;
        }

        bool operator!=( const SkillLevel &b ) const {
            return !( *this == b );
        }
        bool operator<=( const SkillLevel &b ) const {
            return !( *this > b );
        }
        bool operator>=( const SkillLevel &b ) const {
            return !( *this < b );
        }

        bool operator!=( const int &b ) const {
            return !( *this == b );
        }
        bool operator<=( const int &b ) const {
            return !( *this > b );
        }
        bool operator>=( const int &b ) const {
            return !( *this < b );
        }

        void serialize( JsonOut &json ) const;
        void deserialize( const JsonObject &data );
    private:
        // Can be used to counter skill rust when enabled over MAX_SKILL
        int unadjustedLevel() const {
            return _level;
        }

        int unadjustedKnowledgeLevel() const {
            return _knowledgeLevel;
        }

        void on_exercise_change( bool allow_multilevel = false );
};

class SkillLevelMap : public std::map<skill_id, SkillLevel>
{
    public:
        const SkillLevel &get_skill_level_object( const skill_id &ident ) const;
        SkillLevel &get_skill_level_object( const skill_id &ident );
        void mod_skill_level( const skill_id &ident, int delta );
        int get_skill_level( const skill_id &ident ) const;
        int get_skill_level( const skill_id &ident, const item &context ) const;
        float get_progress_level( const skill_id &ident ) const;
        float get_progress_level( const skill_id &ident, const item &context ) const;

        void mod_knowledge_level( const skill_id &ident, int delta );
        int get_knowledge_level( const skill_id &ident ) const;
        int get_knowledge_level( const skill_id &ident, const item &context ) const;
        float get_knowledge_progress_level( const skill_id &ident ) const;
        float get_knowledge_progress_level( const skill_id &ident, const item &context ) const;

        bool meets_skill_requirements( const std::map<skill_id, int> &req ) const;
        bool meets_skill_requirements( const std::map<skill_id, int> &req,
                                       const item &context ) const;
        /** Calculates skill difference
         * @param req Required skills to be compared with.
         * @param context An item to provide context for contextual skills. Can be null.
         * @return Difference in skills. Positive numbers - exceeds; negative - lacks; empty map - no difference.
         */
        std::map<skill_id, int> compare_skill_requirements(
            const std::map<skill_id, int> &req, const item &context ) const;
        std::map<skill_id, int> compare_skill_requirements(
            const std::map<skill_id, int> &req ) const;
        int exceeds_recipe_requirements( const recipe &rec ) const;
        bool theoretical_recipe_requirements( const recipe &rec ) const;
        bool has_recipe_requirements( const recipe &rec ) const;
        bool has_same_levels_as( const SkillLevelMap &other ) const;
};

class SkillDisplayType
{
        friend class string_id<SkillDisplayType>;
        skill_displayType_id _ident;
        translation _display_string;
    public:
        static std::vector<SkillDisplayType> skillTypes;
        static void load( const JsonObject &jsobj );

        static const SkillDisplayType &get_skill_type( const skill_displayType_id & );

        SkillDisplayType();
        SkillDisplayType( const skill_displayType_id &ident, const translation &display_string );

        const skill_displayType_id &ident() const {
            return _ident;
        }
        std::string display_string() const {
            return _display_string.translated();
        }
};

double price_adjustment( int );

#endif // CATA_SRC_SKILL_H
