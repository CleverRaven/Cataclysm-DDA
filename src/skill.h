#pragma once
#ifndef SKILL_H
#define SKILL_H

#include <functional>
#include <map>
#include <set>
#include <vector>

#include "calendar.h"
#include "string_id.h"

class JsonObject;
class JsonIn;
class JsonOut;
class Skill;
class recipe;
class item;
using skill_id = string_id<Skill>;

class Skill
{
        friend class string_id<Skill>;
        skill_id _ident;

        std::string _name;
        std::string _description;
        std::set<std::string> _tags;
        // these are not real skills, they depend on context
        static std::map<skill_id, Skill> contextual_skills;
    public:
        static std::vector<Skill> skills;
        static void load_skill( JsonObject &jsobj );
        // For loading old saves that still have integer-based ids.
        static skill_id from_legacy_int( int legacy_id );
        static skill_id random_skill();

        // clear skill vector, every skill pointer becomes invalid!
        static void reset();

        static std::vector<const Skill *> get_skills_sorted_by(
            std::function<bool ( const Skill &, const Skill & )> pred );

        Skill();
        Skill( skill_id ident, std::string name, std::string description,
               std::set<std::string> tags );

        const skill_id &ident() const {
            return _ident;
        }
        const std::string &name() const {
            return _name;
        }
        const std::string &description() const {
            return _description;
        }

        bool operator==( const Skill &b ) const {
            return this->_ident == b._ident;
        }
        bool operator< ( const Skill &b ) const {
            return this->_ident <  b._ident;    // Only here for the benefit of std::map<Skill,T>
        }

        bool operator!=( const Skill &b ) const {
            return !( *this == b );
        }

        bool is_combat_skill() const;
        bool is_contextual_skill() const;
};

class SkillLevel
{
        int _level = 0;
        int _exercise = 0;
        time_point _lastPracticed = calendar::time_of_cataclysm;
        bool _isTraining = true;
        int _highestLevel = 0;

    public:
        SkillLevel() {}

        bool isTraining() const {
            return _isTraining;
        }
        bool toggleTraining() {
            _isTraining = !_isTraining;
            return _isTraining;
        }

        int level() const {
            return _level;
        }
        int level( int plevel ) {
            _level = plevel;
            if( _level > _highestLevel ) {
                _highestLevel = _level;
            }
            return plevel;
        }

        int highestLevel() const {
            return _highestLevel;
        }

        int exercise( bool raw = false ) const {
            return raw ? _exercise : _exercise / ( ( _level + 1 ) * ( _level + 1 ) );
        }

        int exercised_level() const {
            return level() * level() * 100 + exercise();
        }

        void train( int amount, bool skip_scaling = false );
        bool isRusting() const;
        bool rust( bool charged_bio_mem );
        void practice();
        bool can_train() const;

        void readBook( int minimumGain, int maximumGain, int maximumLevel = -1 );

        bool operator==( const SkillLevel &b ) const {
            return this->_level == b._level && this->_exercise == b._exercise;
        }
        bool operator< ( const SkillLevel &b ) const {
            return this->_level <  b._level || ( this->_level == b._level && this->_exercise < b._exercise );
        }
        bool operator> ( const SkillLevel &b ) const {
            return this->_level >  b._level || ( this->_level == b._level && this->_exercise > b._exercise );
        }

        bool operator==( const int &b ) const {
            return this->_level == b;
        }
        bool operator< ( const int &b ) const {
            return this->_level <  b;
        }
        bool operator> ( const int &b ) const {
            return this->_level >  b;
        }

        bool operator!=( const SkillLevel &b ) const {
            return !( *this == b );
        }
        bool operator<=( const SkillLevel &b ) const {
            return !( *this >  b );
        }
        bool operator>=( const SkillLevel &b ) const {
            return !( *this <  b );
        }

        bool operator!=( const int &b ) const {
            return !( *this == b );
        }
        bool operator<=( const int &b ) const {
            return !( *this >  b );
        }
        bool operator>=( const int &b ) const {
            return !( *this <  b );
        }

        void serialize( JsonOut &jsout ) const;
        void deserialize( JsonIn &jsin );
};

class SkillLevelMap : public std::map<skill_id, SkillLevel>
{
    public:
        const SkillLevel &get_skill_level_object( const skill_id &ident ) const;
        SkillLevel &get_skill_level_object( const skill_id &ident );
        void mod_skill_level( const skill_id &ident, int delta );
        int get_skill_level( const skill_id &ident ) const;
        int get_skill_level( const skill_id &ident, const item &context ) const;

        bool meets_skill_requirements( const std::map<skill_id, int> &req ) const;
        bool meets_skill_requirements( const std::map<skill_id, int> &req,
                                       const item &context ) const;
        /** Calculates skill difference
         * @param req Required skills to be compared with.
         * @param context An item to provide context for contextual skills. Can be null.
         * @return Difference in skills. Positive numbers - exceeds; negative - lacks; empty map - no difference.
         */
        std::map<skill_id, int> compare_skill_requirements(
            const std::map<skill_id, int> &req ) const;
        std::map<skill_id, int> compare_skill_requirements(
            const std::map<skill_id, int> &req, const item &context ) const;
        int exceeds_recipe_requirements( const recipe &rec ) const;
        bool has_recipe_requirements( const recipe &rec ) const;
};

double price_adjustment( int );

#endif
