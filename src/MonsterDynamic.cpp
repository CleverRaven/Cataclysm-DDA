#include "MonsterDynamic.h"

MonsterDynamic::MonsterDynamic()
{
    monster::monster();
    monster::reset_bonuses();
}

MonsterDynamic::MonsterDynamic( monster& mold ) : monster( mold )
{
    monster::reset_bonuses();
}

bool MonsterDynamic::remove_effect( const efftype_id &eff_id, body_part bp = num_bp )
{
    bool rtrn = Creature::remove_effect( eff_id, bp );
    // Become generic if possible.
    for( effid_chk : effects ) {
        if( effects.secound.obj().has_mod() ){
            return rtrn;
        }
    }
    monster mnew = monster( critter );
    g->remove_zombie( critter );
    g->add_zombie( mnew );
    this = mnew;
    return rtrn;
}

int MonsterDynamic::print_info( const catacurses::window &w, int vStart, int vLines, int column ) const
{
    monster::print_info( w, vStart, vLines, column );

    wprintz( w, c_light_gray, _( " It is %s." ), size_names.at( get_size() ) );

    return vStart;
}

void MonsterDynamic::reset_bonuses()
{
    effect_cache.reset();

    BonusUser::reset_bonuses();
    growth_bonus = 0;
}

void MonsterDynamic::process_one_effect( effect &it, bool is_new )
{
    monster::process_one_effect( it, is_new );
    // Monsters don't get trait-based reduction, but they do get effect based reduction
    bool reduced = resists_effect( it );
    const auto get_effect = [&it, is_new]( const std::string & arg, bool reduced ) {
        if( is_new ) {
            return it.get_amount( arg, reduced );
        }
        return it.get_mod( arg, reduced );
    };

    mod_speed_bonus( get_effect( "SPEED", reduced ) );
    mod_dodge_bonus( get_effect( "DODGE", reduced ) );
    mod_hit_bonus( get_effect( "HIT", reduced ) );
    mod_bash_bonus( get_effect( "BASH", reduced ) );
    mod_cut_bonus( get_effect( "CUT", reduced ) );
    mod_growth_bonus( get_effect( "GROWTH", reduced ) );
}

int monster::get_growth_bonus() const
{
    return growth_bonus;
}

void monster::set_growth_bonus( int nsize )
{
    growth_bonus = nsize;
}

void monster::mod_growth_bonus( int nsize )
{
    growth_bonus += nsize;
}

m_size monster::get_size() const
{
    return m_size( type->size + get_growth_bonus() );
}

units::mass monster::get_weight() const
{
    return type->weight * ( static_cast<float>( get_size() ) / type->size );
}

units::volume monster::get_volume() const
{
    return type->volume * ( static_cast<float>( get_size() ) / type->size );
}
