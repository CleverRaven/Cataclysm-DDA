#ifndef SKILL_H
#define SKILL_H

#include "calendar.h"
#include "json.h"

#include <functional>
#include <string>
#include <vector>
#include <set>
#include <iosfwd>

class Skill
{
        size_t _id;
        std::string _ident;

        std::string _name;
        std::string _description;
        std::set<std::string> _tags;

    public:
        static std::vector<Skill> skills;
        static void load_skill(JsonObject &jsobj);
        static const Skill* skill(const std::string& ident);
        static const Skill* skill(size_t id);

        static const Skill* random_skill_with_tag(const std::string& tag);

        static size_t skill_count();
        // clear skill vector, every skill pointer becames invalid!
        static void reset();

        static std::vector<Skill const*> get_skills_sorted_by(
            std::function<bool (Skill const&, Skill const&)> pred);

        Skill();
        Skill(size_t id, std::string ident, std::string name, std::string description,
              std::set<std::string> tags);

        //DEBUG
        size_t id() const
        {
            return _id;
        }

        std::string const& ident() const
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
            return raw ? _exercise : _exercise / (_level + 1);
        }

        int exercised_level() const {
            return level() * 100 + exercise();
        }

        int lastPracticed() const
        {
            return _lastPracticed;
        }

        void train(int amount);
        bool isRusting() const;
        bool rust( bool charged_bio_mem );
        void practice();

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

        SkillLevel &operator= (const SkillLevel &rhs) = default;

        using JsonSerializer::serialize;
        void serialize(JsonOut &jsout) const;
        using JsonDeserializer::deserialize;
        void deserialize(JsonIn &jsin);

        // Make skillLevel act like a raw level by default.
        operator int() const
        {
            return _level;
        }
};

std::istream &operator>>(std::istream &is, SkillLevel &obj); // see savegame_legacy.cpp

double price_adjustment(int);

#endif
