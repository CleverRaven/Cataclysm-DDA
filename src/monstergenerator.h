#ifndef MONSTER_GENERATOR_H
#define MONSTER_GENERATOR_H

#include "json.h"
#include "enums.h"

#include <map>
#include <set>

class Creature;
struct mtype;
enum m_flag : int;
enum monster_trigger : int;
enum m_size : int;
class monster;
class Creature;
struct projectile;
using mon_action_death  = void (*)(monster*);
using mon_action_attack = void (*)(monster*, int);
using mon_action_defend = void (*)(monster*, Creature*, projectile const*);
using mtype_id = std::string;

#define GetMType(x) MonsterGenerator::generator().get_mtype(x)

struct species_type {
    int short_id;
    std::string id;
    std::set<m_flag> flags;
    std::set<monster_trigger> anger_trig, fear_trig, placate_trig;

    species_type(): short_id(0), id("null_species")
    {

    }
    species_type(int _short_id,
                 std::string _id,
                 std::set<m_flag> _flags,
                 std::set<monster_trigger> _anger,
                 std::set<monster_trigger> _fear,
                 std::set<monster_trigger> _placate)
    {
        short_id = _short_id;
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
        static MonsterGenerator &generator()
        {
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

        mtype *get_mtype( const mtype_id& id );
        bool has_mtype( const mtype_id &id ) const;
        bool has_species(const std::string &species) const;
        std::map<mtype_id, mtype *> get_all_mtypes() const;
        std::vector<mtype_id> get_all_mtype_ids() const;
        mtype *get_valid_hallucination();
        friend struct mtype;
    protected:
        m_flag m_flag_from_string( std::string flag ) const;
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
        void init_mf_attitude();

        // data acquisition
        std::set<std::string> get_tags(JsonObject &jo, std::string member);
        std::vector<mon_action_death> get_death_functions(JsonObject &jo, std::string member);
        void load_special_defense(mtype *m, JsonObject &jo, std::string member);
        void load_special_attacks(mtype *m, JsonObject &jo, std::string member);
        template <typename T> std::set<T> get_set_from_tags(std::set<std::string> tags,
                std::map<std::string, T> conversion_map, T fallback);
        template <typename T> T get_from_string(std::string tag, std::map<std::string, T> conversion_map,
                                                T fallback);
        
        

        // finalization
        void apply_species_attributes(mtype *mon);
        void set_mtype_flags(mtype *mon);
        void set_species_ids(mtype *mon);

        template <typename T> void apply_set_to_set(std::set<T> from, std::set<T> &to);

        std::map<mtype_id, mtype *> mon_templates;
        std::map<std::string, species_type *> mon_species;

        std::map<std::string, phase_id> phase_map;
        std::map<std::string, m_size> size_map;
        std::map<std::string, mon_action_death> death_map;
        std::map<std::string, mon_action_attack> attack_map;
        std::map<std::string, mon_action_defend> defense_map;
        std::map<std::string, monster_trigger> trigger_map;
        std::map<std::string, m_flag> flag_map;
};

#endif
