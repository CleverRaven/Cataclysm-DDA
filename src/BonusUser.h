#ifndef BONUSUSER_H
#define BONUSUSER_H

#include "creature.h"

class BonusUser : public virtual Creature
{
    public:
        BonusUser();

        /*
         * Getters for stats - combat-related stats will all be held within
         * the Creature and re-calculated during every normalize() call
         */
        int get_num_blocks() const;
        int get_num_dodges() const;
        virtual int get_num_blocks_bonus() const;
        virtual int get_num_dodges_bonus() const;

        virtual int get_armor_bash_bonus() const;
        virtual int get_armor_cut_bonus() const;
        get_dodge() const;
        float get_hit() const;

        int get_speed() const;
        virtual int get_speed_bonus() const;
        virtual int get_block_bonus() const;
        virtual int get_bash_bonus() const;
        virtual int get_cut_bonus() const;

        virtual float get_dodge_bonus() const;
        virtual float get_hit_bonus() const;

        virtual float get_bash_mult() const;
        virtual float get_cut_mult() const;

        virtual bool get_melee_quiet() const;
        virtual int get_grab_resist() const;
        virtual int get_throw_resist() const;

        /*
         * Setters for stats and bonuses
         */
        virtual void mod_stat( const std::string &stat, float modifier );

        virtual void set_num_blocks_bonus( int nblocks );
        virtual void set_num_dodges_bonus( int ndodges );

        virtual void set_armor_bash_bonus( int nbasharm );
        virtual void set_armor_cut_bonus( int ncutarm );

        virtual void set_speed_bonus( int nspeed );
        virtual void set_block_bonus( int nblock );
        virtual void set_bash_bonus( int nbash );
        virtual void set_cut_bonus( int ncut );

        virtual void mod_speed_bonus( int nspeed );
        virtual void mod_block_bonus( int nblock );
        virtual void mod_bash_bonus( int nbash );
        virtual void mod_cut_bonus( int ncut );

        virtual void set_dodge_bonus( float ndodge );
        virtual void set_hit_bonus( float nhit );

        virtual void mod_dodge_bonus( float ndodge );
        virtual void mod_hit_bonus( float  nhit );

        virtual void set_bash_mult( float nbashmult );
        virtual void set_cut_mult( float ncutmult );

        virtual void set_melee_quiet( bool nquiet );
        virtual void set_grab_resist( int ngrabres );
        virtual void set_throw_resist( int nthrowres );

    protected:
        // used for innate bonuses like effects. weapon bonuses will be
        // handled separately

        int num_blocks; // base number of blocks/dodges per turn
        int num_dodges;
        int num_blocks_bonus; // bonus ""
        int num_dodges_bonus;

        int armor_bash_bonus;
        int armor_cut_bonus;

        int speed_bonus;
        float dodge_bonus;
        int block_bonus;
        float hit_bonus;
        int bash_bonus;
        int cut_bonus;

        float bash_mult;
        float cut_mult;
        bool melee_quiet;

        int grab_resist;
        int throw_resist;
};

#endif // BONUSUSER_H
