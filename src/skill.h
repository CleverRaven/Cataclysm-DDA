#ifndef SKILL_H
#define SKILL_H

#include "calendar.h"
#include "json.h"
#include "string_id.h"

#include <functional>
#include <string>
#include <vector>
#include <set>
#include <iosfwd>

class Skill;
using skill_id = string_id<Skill>;

class Skill
{
        skill_id _ident;

        std::string _name;
        std::string _description;
        std::set<std::string> _tags;

    public:
        static std::vector<Skill> skills;
        static void load_skill(JsonObject &jsobj);
        // For loading old saves that still have integer-based ids.
        static const Skill *from_legacy_int( int legacy_id );

        static const Skill* random_skill_with_tag(const std::string& tag);
        static const Skill* random_skill();

        static size_t skill_count();
        // clear skill vector, every skill pointer becames invalid!
        static void reset();

        static std::vector<Skill const*> get_skills_sorted_by(
            std::function<bool (Skill const&, Skill const&)> pred);

        Skill();
        Skill(skill_id ident, std::string name, std::string description,
              std::set<std::string> tags);

        skill_id const& ident() const
        {
            return _ident;
        }
        std::string const& name() const
        {
            return _name;
        }
        std::string const& description() const
        {
            return _description;
        }

        bool operator==(const Skill &b) const
        {
            return this->_ident == b._ident;
        }
        bool operator< (const Skill &b) const
        {
            return this->_ident <  b._ident;    // Only here for the benefit of std::map<Skill,T>
        }

        bool operator!=(const Skill &b) const
        {
            return !(*this == b);
        }

        bool is_combat_skill() const;
};

class SkillLevel : public JsonSerializer, public JsonDeserializer
{
        int _level;
        int _exercise;
        calendar _lastPracticed;
        bool _isTraining;

    public:
        SkillLevel(int level = 0, int exercise = 0, bool isTraining = true, int lastPracticed = 0);
        SkillLevel(int minLevel, int maxLevel, int minExercise, int maxExercise,
                   bool isTraining = true, int lastPracticed = 0);

        bool isTraining() const
        {
            return _isTraining;
        }
        bool toggleTraining()
        {
            _isTraining = !_isTraining;
            return _isTraining;
        }

        int level() const
        {
            return _level;
        }
        int level(int plevel)
        {
            _level = plevel;
            return plevel;
        }

        int exercise(bool raw = false) const
        {
            return raw ? _exercise : _exercise / ( (_level + 1) * (_level + 1) );
        }

        int exercised_level() const {
            return level() * level() * 100 + exercise();
        }

        int lastPracticed() const
        {
            return _lastPracticed;
        }

        void train(int amount, bool skip_scaling = false);
        bool isRusting() const;
        bool rust( bool charged_bio_mem );
        void practice();
        bool can_train() const;

        void readBook(int minimumGain, int maximumGain, int maximumLevel = -1);

        bool operator==(const SkillLevel &b) const
        {
            return this->_level == b._level && this->_exercise == b._exercise;
        }
        bool operator< (const SkillLevel &b) const
        {
            return this->_level <  b._level || (this->_level == b._level && this->_exercise < b._exercise);
        }
        bool operator> (const SkillLevel &b) const
        {
            return this->_level >  b._level || (this->_level == b._level && this->_exercise > b._exercise);
        }

        bool operator==(const int &b) const
        {
            return this->_level == b;
        }
        bool operator< (const int &b) const
        {
            return this->_level <  b;
        }
        bool operator> (const int &b) const
        {
            return this->_level >  b;
        }

        bool operator!=(const SkillLevel &b) const
        {
            return !(*this == b);
        }
        bool operator<=(const SkillLevel &b) const
        {
            return !(*this >  b);
        }
        bool operator>=(const SkillLevel &b) const
        {
            return !(*this <  b);
        }

        bool operator!=(const int &b) const
        {
            return !(*this == b);
        }
        bool operator<=(const int &b) const
        {
            return !(*this >  b);
        }
        bool operator>=(const int &b) const
        {
            return !(*this <  b);
        }

        SkillLevel &operator= (const SkillLevel &) = default;

        using JsonSerializer::serialize;
        void serialize(JsonOut &jsout) const override;
        using JsonDeserializer::deserialize;
        void deserialize(JsonIn &jsin) override;

        // Make skillLevel act like a raw level by default.
        operator int() const
        {
            return _level;
        }
};

double price_adjustment(int);

#endif
