#ifndef MONSTERDYNAMIC_H
#define MONSTERDYNAMIC_H

#include <monster.h>


class MonsterDynamic : public monster, public BonusUser
{
    public:
        MonsterDynamic();
        MonsterDynamic( monster& mold );

        bool remove_effect( const efftype_id &eff_id, body_part bp = num_bp );

        m_size get_size() const override;
        units::mass get_weight() const override;
        units::volume get_volume() const override;
        int get_growth_bonus() const;
        void set_growth_bonus( int nsize );
        void mod_growth_bonus( int nsize );

    protected:
        int growth_bonus;
    private:
};

#endif // MONSTERDYNAMIC_H
