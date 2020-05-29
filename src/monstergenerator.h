#pragma once
#ifndef CATA_SRC_MONSTERGENERATOR_H
#define CATA_SRC_MONSTERGENERATOR_H

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "enum_bitset.h"
#include "enums.h"
#include "generic_factory.h"
#include "mattack_common.h"
#include "mtype.h"
#include "pimpl.h"
#include "translations.h"
#include "type_id.h"

class Creature;
class JsonObject;
class monster;
struct dealt_projectile_attack;
template <typename T> class string_id;

using mon_action_death  = void ( * )( monster & );
using mon_action_attack = bool ( * )( monster * );
using mon_action_defend = void ( * )( monster &, Creature *, dealt_projectile_attack const * );

struct species_type {
    species_id id;
    bool was_loaded = false;
    translation description;
    translation footsteps;
    enum_bitset<m_flag> flags;
    enum_bitset<mon_trigger> anger;
    enum_bitset<mon_trigger> fear;
    enum_bitset<mon_trigger> placate;
    std::string get_footsteps() const {
        return footsteps.translated();
    }

    species_type(): id( species_id::NULL_ID() ) {

    }

    void load( const JsonObject &jo, const std::string &src );
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
        void load_monster( const JsonObject &jo, const std::string &src );
        void load_species( const JsonObject &jo, const std::string &src );
        void load_monster_attack( const JsonObject &jo, const std::string &src );

        // combines mtype and species information, sets bitflags
        void finalize_mtypes();

        void check_monster_definitions() const;

        const std::vector<mtype> &get_all_mtypes() const;
        mtype_id get_valid_hallucination() const;
        friend struct mtype;
        friend struct species_type;
        friend class mattack_actor;
        std::map<m_flag, int> m_flag_usage_stats;

    private:
        MonsterGenerator();

        // Init functions
        void init_phases();
        void init_death();
        void init_attack();
        void init_defense();

        void add_hardcoded_attack( const std::string &type, mon_action_attack f );
        void add_attack( std::unique_ptr<mattack_actor> );
        void add_attack( const mtype_special_attack &wrapper );

        /** Gets an actor object without saving it anywhere */
        mtype_special_attack create_actor( const JsonObject &obj, const std::string &src ) const;

        // finalization
        void apply_species_attributes( mtype &mon );
        void set_species_ids( mtype &mon );
        void finalize_pathfinding_settings( mtype &mon );

        friend class string_id<mtype>;
        friend class string_id<species_type>;
        friend class string_id<mattack_actor>;

        pimpl<generic_factory<mtype>> mon_templates;
        pimpl<generic_factory<species_type>> mon_species;
        std::vector<mtype_id> hallucination_monsters;

        std::map<std::string, phase_id> phase_map;
        std::map<std::string, mon_action_death> death_map;
        std::map<std::string, mon_action_defend> defense_map;
        std::map<std::string, mtype_special_attack> attack_map;
};

void load_monster_adjustment( const JsonObject &jsobj );
void reset_monster_adjustment();

#endif // CATA_SRC_MONSTERGENERATOR_H
