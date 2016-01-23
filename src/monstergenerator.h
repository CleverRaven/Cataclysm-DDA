#ifndef MONSTER_GENERATOR_H
#define MONSTER_GENERATOR_H

#include "json.h"
#include "enums.h"
#include "string_id.h"

#include <map>
#include <memory>
#include <set>

class Creature;
struct mtype;
enum m_flag : int;
enum monster_trigger : int;
enum m_size : int;
class monster;
class Creature;
struct projectile;
struct dealt_projectile_attack;
using mon_action_death  = void (*)(monster*);
using mon_action_attack = bool (*)(monster*);
using mon_action_defend = void (*)(monster&, Creature*, dealt_projectile_attack const*);
using mtype_id = string_id<mtype>;
struct species_type;
using species_id = string_id<species_type>;
template<typename T>
class generic_factory;

struct species_type {
    species_id id;
    bool was_loaded = false;
    std::set<m_flag> flags;
    std::set<monster_trigger> anger_trig, fear_trig, placate_trig;

    species_type(): id( NULL_ID ) {

    }

    void load( JsonObject &jo );
};

class MonsterGenerator
{
    public:
        static MonsterGenerator &generator() {
            static MonsterGenerator generator;

            return generator;
        }
        ~MonsterGenerator();

        // clear monster & species definitions
        void reset();

        // JSON loading functions
        void load_monster( JsonObject &jo );
        void load_species( JsonObject &jo );

        // combines mtype and species information, sets bitflags
        void finalize_mtypes();


        void check_monster_definitions() const;

        std::vector<const mtype *> get_all_mtypes() const;
        mtype_id get_valid_hallucination() const;
        friend struct mtype;
        friend struct species_type;

    protected:
        m_flag m_flag_from_string( std::string flag ) const;
    private:
        /** Default constructor */
        MonsterGenerator();

        // Init functions
        void init_phases();
        void init_death();
        void init_attack();
        void init_defense();
        void init_trigger();
        void init_flags();
        void init_mf_attitude();

        // data acquisition
        std::set<std::string> get_tags( JsonObject &jo, std::string member );
        std::vector<mon_action_death> get_death_functions( JsonObject &jo, std::string member );
        void load_special_defense( mtype *m, JsonObject &jo, std::string member );
        void load_special_attacks( mtype *m, JsonObject &jo, std::string member );
        template <typename T> std::set<T> get_set_from_tags( std::set<std::string> tags,
                std::map<std::string, T> conversion_map, T fallback );
        template <typename T> T get_from_string( std::string tag, std::map<std::string, T> conversion_map,
                T fallback );

        // finalization
        void apply_species_attributes( mtype &mon );
        void set_mtype_flags( mtype &mon );
        void set_species_ids( mtype &mon );

        template <typename T> void apply_set_to_set( std::set<T> from, std::set<T> &to );

        friend class string_id<mtype>;
        friend class string_id<species_type>;

        // Using unique_ptr here to avoid including generic_factory.h in this header.
        std::unique_ptr<generic_factory<mtype>> mon_templates;
        std::unique_ptr<generic_factory<species_type>> mon_species;

        std::map<std::string, phase_id> phase_map;
        std::map<std::string, mon_action_death> death_map;
        std::map<std::string, mon_action_attack> attack_map;
        std::map<std::string, mon_action_defend> defense_map;
        std::map<std::string, monster_trigger> trigger_map;
        std::map<std::string, m_flag> flag_map;
};

#endif
