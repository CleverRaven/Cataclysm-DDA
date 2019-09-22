#include "BonusUser.h"

BonusUser::BonusUser() : BonusUser::BonusUser()
{
    reset_bonuses();
}

BonusUser::reset_bonuses()
{
    num_blocks = 1; // + (BonusUser::get_num_blocks)
    num_dodges = 1; //+ (BonusUser::get_num_dodges)
    num_blocks_bonus = 0; // + ''
    num_dodges_bonus = 0; // + ''

    armor_bash_bonus = 0; // - (monster::get_armor_bash)
    armor_cut_bonus = 0; // - (monster::get_armor_cut)

    speed_bonus = 0; // + (BonusUser::get_speed)
    dodge_bonus = 0; // + (BonusUser::get_dodge)
    block_bonus = 0; // + (BonusUser::get_block)
    hit_bonus = 0; // + (BonusUser::get_hit)
    bash_bonus = 0; // +
    cut_bonus = 0; // +

    bash_mult = 1.0f; // +
    cut_mult = 1.0f; // +

    melee_quiet = false; // +
    grab_resist = 0; // +
    throw_resist = 0; // +
}

void BonusUser::reset_stats()
{
    // Nothing here yet
}

int BonusUser::get_num_blocks() const
{
    return num_blocks + num_blocks_bonus;
}
int BonusUser::get_num_dodges() const
{
    return num_dodges + num_dodges_bonus;
}
int BonusUser::get_num_blocks_bonus() const
{
    return num_blocks_bonus;
}
int BonusUser::get_num_dodges_bonus() const
{
    return num_dodges_bonus;
}

int BonusUser::get_armor_bash( body_part ) const
{
    return armor_bash_bonus;
}
int BonusUser::get_armor_cut( body_part ) const
{
    return armor_cut_bonus;
}
int BonusUser::get_armor_bash_base( body_part ) const
{
    return armor_bash_bonus;
}
int BonusUser::get_armor_cut_base( body_part ) const
{
    return armor_cut_bonus;
}
int BonusUser::get_armor_bash_bonus() const
{
    return armor_bash_bonus;
}
int BonusUser::get_armor_cut_bonus() const
{
    return armor_cut_bonus;
}

int BonusUser::get_speed() const
{
    return get_speed_base() + get_speed_bonus();
}
float BonusUser::get_dodge() const
{
    return get_dodge_base() + get_dodge_bonus();
}
float BonusUser::get_hit() const
{
    return get_hit_base() + get_hit_bonus();
}

int BonusUser::get_speed_bonus() const
{
    return speed_bonus;
}

float BonusUser::get_dodge_bonus() const
{
    return dodge_bonus;
}

float BonusUser::get_hit_bonus() const
{
    return hit_bonus; //base is 0
}
int BonusUser::get_bash_bonus() const
{
    return bash_bonus;
}
int BonusUser::get_cut_bonus() const
{
    return cut_bonus;
}

void BonusUser::mod_stat( const std::string &stat, float modifier )
{
    if( stat == "speed" ) {
        mod_speed_bonus( modifier );
    } else if( stat == "dodge" ) {
        mod_dodge_bonus( modifier );
    } else if( stat == "block" ) {
        mod_block_bonus( modifier );
    } else if( stat == "hit" ) {
        mod_hit_bonus( modifier );
    } else if( stat == "bash" ) {
        mod_bash_bonus( modifier );
    } else if( stat == "cut" ) {
        mod_cut_bonus( modifier );
    } else if( stat == "pain" ) {
        mod_pain( modifier );
    } else if( stat == "moves" ) {
        mod_moves( modifier );
    } else {
        add_msg( "Tried to modify a nonexistent stat %s.", stat.c_str() );
    }
}

void BonusUser::set_num_blocks_bonus( int nblocks )
{
    num_blocks_bonus = nblocks;
}
void BonusUser::set_num_dodges_bonus( int ndodges )
{
    num_dodges_bonus = ndodges;
}

void BonusUser::set_armor_bash_bonus( int nbasharm )
{
    armor_bash_bonus = nbasharm;
}
void BonusUser::set_armor_cut_bonus( int ncutarm )
{
    armor_cut_bonus = ncutarm;
}

void BonusUser::set_speed_bonus( int nspeed )
{
    speed_bonus = nspeed;
}
void BonusUser::set_dodge_bonus( float ndodge )
{
    dodge_bonus = ndodge;
}

void BonusUser::set_hit_bonus( float nhit )
{
    hit_bonus = nhit;
}
void BonusUser::set_bash_bonus( int nbash )
{
    bash_bonus = nbash;
}
void BonusUser::set_cut_bonus( int ncut )
{
    cut_bonus = ncut;
}
void BonusUser::mod_speed_bonus( int nspeed )
{
    speed_bonus += nspeed;
}
void BonusUser::mod_dodge_bonus( float ndodge )
{
    dodge_bonus += ndodge;
}
void BonusUser::mod_block_bonus( int nblock )
{
    block_bonus += nblock;
}
void BonusUser::mod_hit_bonus( float nhit )
{
    hit_bonus += nhit;
}
void BonusUser::mod_bash_bonus( int nbash )
{
    bash_bonus += nbash;
}
void BonusUser::mod_cut_bonus( int ncut )
{
    cut_bonus += ncut;
}

void BonusUser::set_bash_mult( float nbashmult )
{
    bash_mult = nbashmult;
}
void BonusUser::set_cut_mult( float ncutmult )
{
    cut_mult = ncutmult;
}

void BonusUser::set_melee_quiet( bool nquiet )
{
    melee_quiet = nquiet;
}
void BonusUser::set_grab_resist( int ngrabres )
{
    grab_resist = ngrabres;
}
void BonusUser::set_throw_resist( int nthrowres )
{
    throw_resist = nthrowres;
}
