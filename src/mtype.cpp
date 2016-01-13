#include "mtype.h"
#include "creature.h"
#include "translations.h"
#include "monstergenerator.h"
#include "mondeath.h"
#include "field.h"

#include <algorithm>

const species_id MOLLUSK( "MOLLUSK" );

mtype::mtype ()
{
    id = NULL_ID;
    name = "human";
    name_plural = "humans";
    description = "";
    sym = " ";
    color = c_white;
    size = MS_MEDIUM;
    mat = {"hflesh"};
    phase = SOLID;
    difficulty = 0;
    agro = 0;
    morale = 0;
    speed = 0;
    attack_cost = 100;
    melee_skill = 0;
    melee_dice = 0;
    melee_sides = 0;
    melee_cut = 0;
    sk_dodge = 0;
    armor_bash = 0;
    armor_cut = 0;
    armor_stab = 0;
    armor_acid = 0;
    armor_fire = 0;
    hp = 0;
    def_chance = 0;
    upgrades = false;
    half_life = -1;
    upgrade_into = NULL_ID;
    upgrade_group = NULL_ID;
    dies.push_back(&mdeath::normal);
    sp_defense = nullptr;
    luminance = 0;
    flags.insert(MF_HUMAN);
    flags.insert(MF_BONES);
    flags.insert(MF_LEATHER);
}

std::string mtype::nname(unsigned int quantity) const
{
    return ngettext(name.c_str(), name_plural.c_str(), quantity);
}

bool mtype::has_special_attack( const std::string &attack_name ) const
{
    return special_attacks.find( attack_name ) != special_attacks.end();
}

bool mtype::has_flag(m_flag flag) const
{
    return bitflags[flag];
}

bool mtype::has_flag(std::string flag) const
{
    return has_flag( MonsterGenerator::generator().m_flag_from_string( flag ) );
}

void mtype::set_flag(std::string flag, bool state)
{
    if( state ) {
        flags.insert( MonsterGenerator::generator().m_flag_from_string( flag ) );
    } else {
        flags.erase( MonsterGenerator::generator().m_flag_from_string( flag ) );
    }
}

bool mtype::has_material( const std::string &material ) const
{
    return std::find( mat.begin(), mat.end(),  material ) != mat.end();
}

bool mtype::has_anger_trigger(monster_trigger trig) const
{
    return bitanger[trig];
}

bool mtype::has_fear_trigger(monster_trigger trig) const
{
    return bitfear[trig];
}

bool mtype::has_placate_trigger(monster_trigger trig) const
{
    return bitplacate[trig];
}

bool mtype::in_category(std::string category) const
{
    return (categories.find(category) != categories.end());
}

bool mtype::in_species( const species_id &spec ) const
{
    return species.count( spec ) > 0;
}

bool mtype::in_species( const species_type &spec ) const
{
    return species_ptrs.count( &spec ) > 0;
}

bool mtype::same_species( const mtype &other ) const
{
    for( auto &s : species_ptrs ) {
        if( other.in_species( *s ) ) {
            return true;
        }
    }
    return false;
}

field_id mtype::bloodType() const
{
    if (has_flag(MF_ACID_BLOOD))
        //A monster that has the death effect "ACID" does not need to have acid blood.
    {
        return fd_acid;
    }
    if (has_flag(MF_BILE_BLOOD)) {
        return fd_bile;
    }
    if (has_flag(MF_LARVA) || has_flag(MF_ARTHROPOD_BLOOD)) {
        return fd_blood_invertebrate;
    }
    if( has_material("veggy") ) {
        return fd_blood_veggy;
    }
    if( has_material("iflesh") ) {
        return fd_blood_insect;
    }
    if( has_flag( MF_WARM ) && has_material("flesh") ) {
        return fd_blood;
    }
    return fd_null;
}

field_id mtype::gibType() const
{
    if (has_flag(MF_LARVA) || in_species( MOLLUSK )) {
        return fd_gibs_invertebrate;
    }
    if( has_material("veggy") ) {
        return fd_gibs_veggy;
    }
    if( has_material("iflesh") ) {
        return fd_gibs_insect;
    }
    if( has_material("flesh") ) {
        return fd_gibs_flesh;
    }
    // There are other materials not listed here like steel, protoplasmic, powder, null, stone, bone
    return fd_null;
}

itype_id mtype::get_meat_itype() const
{
    if( has_flag( MF_POISON ) ) {
        if( has_material("flesh") || has_material("hflesh") ) {
            return "meat_tainted";
        } else if( has_material("iflesh") ) {
            //In the future, insects could drop insect flesh rather than plain ol' meat.
            return "meat_tainted";
        } else if( has_material("veggy") ) {
            return "veggy_tainted";
        }
    } else {
        if( has_material("flesh") || has_material("hflesh") ) {
            if( has_flag( MF_HUMAN ) ) {
                return "human_flesh";
            } else if( has_flag( MF_AQUATIC ) ) {
                return "fish";
            } else {
                return "meat";
            }
        } else if( has_material("bone") ) {
            return "bone_tainted";
        } else if( has_material("iflesh") ) {
            //In the future, insects could drop insect flesh rather than plain ol' meat.
            return "meat";
        } else if( has_material("veggy") ) {
            return "veggy";
        }
    }
    return "null";
}

bool mtype_special_attack::call( monster &mon ) const
{
    if( function_type == ATTACK_CPP ) {
        return cpp_function( &mon );
    } else if( function_type == ATTACK_ACTOR_PTR ) {
        return actor_ptr->call( mon );
    }

    return false;
}

mtype_special_attack::~mtype_special_attack()
{
    if( function_type == ATTACK_ACTOR_PTR ) {
        delete actor_ptr;
    }
}

mtype_special_attack::mtype_special_attack( const mtype_special_attack &other )
    : function_type( other.function_type ), cooldown( other.cooldown )
{
    if( function_type == ATTACK_CPP ) {
        cpp_function = other.cpp_function;
    } else if( function_type == ATTACK_ACTOR_PTR ) {
        actor_ptr = other.actor_ptr->clone();
    }
}

void mtype_special_attack::operator=( const mtype_special_attack &other )
{
    this->~mtype_special_attack();
    new (this) mtype_special_attack( other );
}

void mtype_special_attack::set_cooldown( int i )
{
    cooldown = i;
}
