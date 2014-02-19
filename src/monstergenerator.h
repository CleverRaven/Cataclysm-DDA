#ifndef MONSTER_GENERATOR_H
#define MONSTER_GENERATOR_H

#include "json.h"
#include "mtype.h"

#include <map>
#include <set>

typedef void (mdeath::*MonDeathFunction)(monster*);
typedef void (mattack::*MonAttackFunction)(monster*);
typedef void (mdefense::*MonDefenseFunction)(monster*);

#define GetMType(x) MonsterGenerator::generator().get_mtype(x)

struct species_type
{
    std::string id;
    std::set<m_flag> flags;
    std::set<monster_trigger> anger_trig, fear_trig, placate_trig;

    species_type():id("null_species")
    {

    }
    species_type(std::string _id,
                 std::set<m_flag> _flags,
                 std::set<monster_trigger> _anger,
                 std::set<monster_trigger> _fear,
                 std::set<monster_trigger> _placate)
    {
        id = _id;
        flags = _flags;
        anger_trig = _anger;
        fear_trig = _fear;
        placate_trig = _placate;
    }
};

class MonsterGenerator
{
    public:
        static MonsterGenerator &generator(){
            static MonsterGenerator generator;

            return generator;
        }
        /** Default destructor */
        virtual ~MonsterGenerator();

        // clear monster & species definitions
        void reset();

        // JSON loading functions
        void load_monster(JsonObject &jo);
        void load_species(JsonObject &jo);

        // combines mtype and species information, sets bitflags
        void finalize_mtypes();

        void check_monster_definitions() const;

        mtype *get_mtype(std::string mon);
        mtype *get_mtype(int mon);
        bool has_mtype(const std::string& mon) const;
        bool has_species(const std::string& species) const;
        std::map<std::string, mtype*> get_all_mtypes() const;
        std::vector<std::string> get_all_mtype_ids() const;
        mtype *get_valid_hallucination();
    protected:
    private:
        /** Default constructor */
        MonsterGenerator();

        // Init functions
        void init_phases();
        void init_sizes();
        void init_death();
        void init_attack();
        void init_defense();
        void init_trigger();
        void init_flags();

        // data acquisition
        std::set<std::string> get_tags(JsonObject &jo, std::string member);
        std::vector<void (mdeath::*)(monster*)> get_death_functions(JsonObject &jo, std::string member);
        MonAttackFunction get_attack_function(JsonObject &jo, std::string member);
        MonDefenseFunction get_defense_function(JsonObject &jo, std::string member);
        template <typename T> std::set<T> get_set_from_tags(std::set<std::string> tags, std::map<std::string, T> conversion_map, T fallback);
        template <typename T> T get_from_string(std::string tag, std::map<std::string, T> conversion_map, T fallback);

        // finalization
        void apply_species_attributes(mtype *mon);
        void set_mtype_flags(mtype *mon);

        template <typename T> void apply_set_to_set(std::set<T> from, std::set<T> &to);

        std::map<std::string, mtype*> mon_templates;
        std::map<std::string, species_type*> mon_species;

        std::map<std::string, phase_id> phase_map;
        std::map<std::string, m_size> size_map;
        std::map<std::string, MonDeathFunction> death_map;
        std::map<std::string, MonAttackFunction> attack_map;
        std::map<std::string, MonDefenseFunction> defense_map;
        std::map<std::string, monster_trigger> trigger_map;
        std::map<std::string, m_flag> flag_map;
};

#endif // MONSTER_GENERATOR_H
