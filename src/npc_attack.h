#pragma once
#ifndef CATA_SRC_NPC_ATTACK_H
#define CATA_SRC_NPC_ATTACK_H

#include <optional>
#include <vector>

#include "gun_mode.h"
#include "map_iterator.h"
#include "point.h"
#include "type_id.h"

class Creature;
class item;
class npc;

class npc_attack_rating
{
        // the total calculated effectiveness of this attack. no value means this attack is not possible.
        std::optional<int> _value = std::nullopt;
        // the target tile of the attack
        tripoint _target;
    public:
        npc_attack_rating() = default;
        npc_attack_rating( const std::optional<int> &_value,
                           const tripoint &_target ) : _value( _value ), _target( _target ) {}
        std::optional<int> value() const {
            return _value;
        }
        tripoint target() const {
            return _target;
        }
        bool operator>( const npc_attack_rating &rhs ) const;
        bool operator>( int rhs ) const;
        bool operator<( int rhs ) const;
        npc_attack_rating operator-=( int rhs );
};

class npc_attack
{
    public:
        /**
         *  Parameters:
         *  source - the npc making the attack
         *  target - the desired target of the npc. valued at 100%
         */
        virtual npc_attack_rating evaluate( const npc &source, const Creature *target ) const = 0;
        virtual void use( npc &source, const tripoint &location ) const = 0;
        /**
         *  For debug information. returns all evaluated effectivenesses.
         *  This is abstracted out in evaluate() as it's faster to not use a container
         */
        virtual std::vector<npc_attack_rating> all_evaluations( const npc &source,
                const Creature *target ) const = 0;

        virtual ~npc_attack() = default;
};

/**
 *  The type of npc_attack that is a spell.
 *  Assumption: if we made it this far, the npc is able to cast the spell.
 */
class npc_attack_spell : public npc_attack
{
        const spell_id attack_spell_id;
    public:
        explicit npc_attack_spell( const spell_id &attack_spell_id ) : attack_spell_id( attack_spell_id ) {}
        npc_attack_rating evaluate( const npc &source, const Creature *target ) const override;
        std::vector<npc_attack_rating> all_evaluations( const npc &source,
                const Creature *target ) const override;
        void use( npc &source, const tripoint &location ) const override;
    private:
        bool can_use( const npc &source ) const;
        int base_time_penalty( const npc &source ) const;
        npc_attack_rating evaluate_tripoint(
            const npc &source, const Creature *target, const tripoint &location ) const;
};

class npc_attack_melee : public npc_attack
{
        item &weapon;
    public:
        explicit npc_attack_melee( item &weapon ) : weapon( weapon ) {}
        npc_attack_rating evaluate( const npc &source, const Creature *target ) const override;
        std::vector<npc_attack_rating> all_evaluations( const npc &source,
                const Creature *target ) const override;
        void use( npc &source, const tripoint &location ) const override;
    private:
        tripoint_range<tripoint> targetable_points( const npc &source ) const;
        bool can_use( const npc &source ) const;
        int base_time_penalty( const npc &source ) const;
        npc_attack_rating evaluate_critter( const npc &source, const Creature *target,
                                            Creature *critter ) const;
};

class npc_attack_gun : public npc_attack
{
        const gun_mode gunmode;
        item &gun;
    public:
        explicit npc_attack_gun( item &gun, const gun_mode &gunmode ) : gunmode( gunmode ), gun( gun ) {}
        npc_attack_rating evaluate( const npc &source, const Creature *target ) const override;
        std::vector<npc_attack_rating> all_evaluations( const npc &source,
                const Creature *target ) const override;
        void use( npc &source, const tripoint &location ) const override;
    private:
        tripoint_range<tripoint> targetable_points( const npc &source ) const;
        bool can_use( const npc &source ) const;
        int base_time_penalty( const npc &source ) const;
        npc_attack_rating evaluate_tripoint(
            const npc &source, const Creature *target, const tripoint &location ) const;
};

class npc_attack_throw : public npc_attack
{
        item &thrown_item;
    public:
        explicit npc_attack_throw( item &thrown_item ) : thrown_item( thrown_item ) {}
        npc_attack_rating evaluate( const npc &source, const Creature *target ) const override;
        std::vector<npc_attack_rating> all_evaluations( const npc &source,
                const Creature *target ) const override;
        void use( npc &source, const tripoint &location ) const override;
    private:
        tripoint_range<tripoint> targetable_points( const npc &source ) const;
        bool can_use( const npc &source ) const;
        int base_penalty( const npc &source ) const;
        npc_attack_rating evaluate_tripoint(
            const npc &source, const Creature *target, const tripoint &location ) const;
};

class npc_attack_activate_item : public npc_attack
{
        item &activatable_item;
    public:
        explicit npc_attack_activate_item( item &activatable_item ) :
            activatable_item( activatable_item ) {}
        npc_attack_rating evaluate( const npc &source, const Creature *target ) const override;
        std::vector<npc_attack_rating> all_evaluations( const npc &source,
                const Creature *target ) const override;
        void use( npc &source, const tripoint &location ) const override;
    private:
        bool can_use( const npc &source ) const;
};

#endif // CATA_SRC_NPC_ATTACK_H
