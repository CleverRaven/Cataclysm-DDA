#pragma once
#ifndef MONSTER_GENERATOR_H
#define MONSTER_GENERATOR_H

#include "enums.h"
#include "string_id.h"
#include "mattack_common.h"
#include "pimpl.h"

#include <map>
#include <memory>
#include <vector>
#include <set>

class JsonObject;
class Creature;
struct mtype;
enum m_flag : int;
enum monster_trigger : int;
enum m_size : int;
class monster;
class Creature;
struct dealt_projectile_attack;
using mon_action_death  = void ( * )( monster & );
using mon_action_attack = bool ( * )( monster * );
using mon_action_defend = void ( * )( monster &, Creature *, dealt_projectile_attack const * );
using mtype_id = string_id<mtype>;
struct species_type;
using species_id = string_id<species_type>;

class mattack_actor;
template<typename T>
class generic_factory;

struct species_type {
    species_id id;
    bool was_loaded = false;
    std::set<m_flag> flags;
    std::set<monster_trigger> anger_trig, fear_trig, placate_trig;

    species_type(): id( species_id::NULL_ID() ) {

    }

    void load( JsonObject &jo, const std::string &src );
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
        void load_monster( JsonObject &jo, const std::string &src );
        void load_species( JsonObject &jo, const std::string &src );
        void load_monster_attack( JsonObject &jo, const std::string &src );

        // combines mtype and species information, sets bitflags
        void finalize_mtypes();


        void check_monster_definitions() const;

        const std::vector<mtype> &get_all_mtypes() const;
        mtype_id get_valid_hallucination() const;
        friend struct mtype;
        friend struct species_type;
        friend class mattack_actor;

    protected:
        m_flag m_flag_from_string( const std::string &flag ) const;
    private:
        MonsterGenerator();

        // Init functions
        void init_phases();
        void init_death();
        void init_attack();
        void init_defense();
        void init_trigger();
        void init_flags();
        void init_mf_attitude();

        void add_hardcoded_attack( const std::string &type, const mon_action_attack f );
        void add_attack( mattack_actor *ptr );
        void add_attack( const mtype_special_attack &wrapper );

        /** Gets an actor object without saving it anywhere */
        mtype_special_attack create_actor( JsonObject obj, const std::string &src ) const;

        // finalization
        void apply_species_attributes( mtype &mon );
        void set_mtype_flags( mtype &mon );
        void set_species_ids( mtype &mon );
        void finalize_pathfinding_settings( mtype &mon );

        template <typename T> void apply_set_to_set( std::set<T> from, std::set<T> &to );

        friend class string_id<mtype>;
        friend class string_id<species_type>;
        friend class string_id<mattack_actor>;

        pimpl<generic_factory<mtype>> mon_templates;
        pimpl<generic_factory<species_type>> mon_species;
        std::vector<mtype_id> hallucination_monsters;

        std::map<std::string, phase_id> phase_map;
        std::map<std::string, mon_action_death> death_map;
        std::map<std::string, mon_action_defend> defense_map;
        std::map<std::string, monster_trigger> trigger_map;
        std::map<std::string, mtype_special_attack> attack_map;
        std::map<std::string, m_flag> flag_map;
};

#endif
