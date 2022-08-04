#include "bionics.h"
#include "character.h"
#include "flag.h"
#include "itype.h"
#include "make_static.h"
#include "map.h"
#include "memorial_logger.h"
#include "mutation.h"
#include "output.h"
#include "weakpoint.h"

static const bionic_id bio_ads( "bio_ads" );

static const json_character_flag json_flag_SEESLEEP( "SEESLEEP" );

bool Character::can_interface_armor() const
{
    bool okay = std::any_of( my_bionics->begin(), my_bionics->end(),
    []( const bionic & b ) {
        return b.powered && b.info().has_flag( STATIC( json_character_flag( "BIONIC_ARMOR_INTERFACE" ) ) );
    } );
    return okay;
}

resistances Character::mutation_armor( bodypart_id bp ) const
{
    resistances res;
    for( const trait_id &iter : get_mutations() ) {
        res += iter->damage_resistance( bp );
    }

    return res;
}

float Character::mutation_armor( bodypart_id bp, damage_type dt ) const
{
    return mutation_armor( bp ).type_resist( dt );
}

float Character::mutation_armor( bodypart_id bp, const damage_unit &du ) const
{
    return mutation_armor( bp ).get_effective_resist( du );
}

int Character::get_armor_bash( bodypart_id bp ) const
{
    return get_armor_bash_base( bp ) + armor_bash_bonus;
}

int Character::get_armor_cut( bodypart_id bp ) const
{
    return get_armor_cut_base( bp ) + armor_cut_bonus;
}

int Character::get_armor_bullet( bodypart_id bp ) const
{
    return get_armor_bullet_base( bp ) + armor_bullet_bonus;
}

// TODO: Reduce duplication with below function
int Character::get_armor_type( damage_type dt, bodypart_id bp ) const
{
    switch( dt ) {
        case damage_type::PURE:
        case damage_type::BIOLOGICAL:
            return 0;
        case damage_type::BASH:
            return get_armor_bash( bp );
        case damage_type::CUT:
            return get_armor_cut( bp );
        case damage_type::STAB:
            return get_armor_cut( bp ) * 0.8f;
        case damage_type::BULLET:
            return get_armor_bullet( bp );
        case damage_type::ACID:
        case damage_type::HEAT:
        case damage_type::COLD:
        case damage_type::ELECTRIC: {
            return worn.damage_resist( dt, bp ) + mutation_armor( bp, dt ) + bp->damage_resistance( dt );
        }
        case damage_type::NONE:
        case damage_type::NUM:
            // Let it error below
            break;
    }

    debugmsg( "Invalid damage type: %d", dt );
    return 0;
}

std::map<bodypart_id, int> Character::get_all_armor_type( damage_type dt,
        const std::map<bodypart_id, std::vector<const item *>> &clothing_map ) const
{
    std::map<bodypart_id, int> ret;
    for( const bodypart_id &bp : get_all_body_parts() ) {
        ret.emplace( bp, 0 );
    }

    for( std::pair<const bodypart_id, int> &per_bp : ret ) {
        const bodypart_id &bp = per_bp.first;
        switch( dt ) {
            case damage_type::PURE:
            case damage_type::BIOLOGICAL:
                // Characters cannot resist this
                return ret;
            /* BASH, CUT, STAB, and BULLET don't benefit from the clothing_map optimization */
            // TODO: Fix that
            case damage_type::BASH:
                per_bp.second += get_armor_bash( bp );
                break;
            case damage_type::CUT:
                per_bp.second += get_armor_cut( bp );
                break;
            case damage_type::STAB:
                per_bp.second += get_armor_cut( bp ) * 0.8f;
                break;
            case damage_type::BULLET:
                per_bp.second += get_armor_bullet( bp );
                break;
            case damage_type::ACID:
            case damage_type::HEAT:
            case damage_type::COLD:
            case damage_type::ELECTRIC: {
                for( const item *it : clothing_map.at( bp ) ) {
                    per_bp.second += it->damage_resist( dt, false, bp );
                }

                per_bp.second += mutation_armor( bp, dt );
                per_bp.second += bp->damage_resistance( dt );
                break;
            }
            case damage_type::NONE:
            case damage_type::NUM:
                debugmsg( "Invalid damage type: %d", dt );
                return ret;
        }
    }

    return ret;
}

int Character::get_armor_bash_base( bodypart_id bp ) const
{
    float ret = worn.damage_resist( damage_type::BASH, bp );
    for( const bionic_id &bid : get_bionics() ) {
        const auto bash_prot = bid->bash_protec.find( bp.id() );
        if( bash_prot != bid->bash_protec.end() ) {
            ret += bash_prot->second;
        }
    }

    ret += mutation_armor( bp, damage_type::BASH );
    ret += bp->damage_resistance( damage_type::BASH );
    return ret;
}

int Character::get_armor_cut_base( bodypart_id bp ) const
{
    float ret = worn.damage_resist( damage_type::CUT, bp );

    for( const bionic_id &bid : get_bionics() ) {
        const auto cut_prot = bid->cut_protec.find( bp.id() );
        if( cut_prot != bid->cut_protec.end() ) {
            ret += cut_prot->second;
        }
    }

    ret += mutation_armor( bp, damage_type::CUT );
    ret += bp->damage_resistance( damage_type::CUT );
    return ret;
}

int Character::get_armor_bullet_base( bodypart_id bp ) const
{
    float ret = worn.damage_resist( damage_type::BULLET, bp );

    for( const bionic_id &bid : get_bionics() ) {
        const auto bullet_prot = bid->bullet_protec.find( bp.id() );
        if( bullet_prot != bid->bullet_protec.end() ) {
            ret += bullet_prot->second;
        }
    }

    ret += mutation_armor( bp, damage_type::BULLET );
    ret += bp->damage_resistance( damage_type::BULLET );
    return ret;
}

int Character::get_armor_acid( bodypart_id bp ) const
{
    return get_armor_type( damage_type::ACID, bp );
}

int Character::get_env_resist( bodypart_id bp ) const
{
    float ret = worn.get_env_resist( bp );

    for( const bionic_id &bid : get_bionics() ) {
        const auto EP = bid->env_protec.find( bp.id() );
        if( EP != bid->env_protec.end() ) {
            ret += EP->second;
        }
    }

    if( bp == body_part_eyes && has_flag( json_flag_SEESLEEP ) ) {
        ret += 8;
    }
    return ret;
}

std::map<bodypart_id, int> Character::get_armor_fire( const
        std::map<bodypart_id, std::vector<const item *>> &clothing_map ) const
{
    return get_all_armor_type( damage_type::HEAT, clothing_map );
}

// adjusts damage unit depending on type by enchantments.
// the ITEM_ enchantments only affect the damage resistance for that one item, while the others affect all of them
static void armor_enchantment_adjust( Character &guy, damage_unit &du )
{
    switch( du.type ) {
        case damage_type::ACID:
            du.amount = guy.calculate_by_enchantment( du.amount, enchant_vals::mod::ARMOR_ACID );
            break;
        case damage_type::BASH:
            du.amount = guy.calculate_by_enchantment( du.amount, enchant_vals::mod::ARMOR_BASH );
            break;
        case damage_type::BIOLOGICAL:
            du.amount = guy.calculate_by_enchantment( du.amount, enchant_vals::mod::ARMOR_BIO );
            break;
        case damage_type::COLD:
            du.amount = guy.calculate_by_enchantment( du.amount, enchant_vals::mod::ARMOR_COLD );
            break;
        case damage_type::CUT:
            du.amount = guy.calculate_by_enchantment( du.amount, enchant_vals::mod::ARMOR_CUT );
            break;
        case damage_type::ELECTRIC:
            du.amount = guy.calculate_by_enchantment( du.amount, enchant_vals::mod::ARMOR_ELEC );
            break;
        case damage_type::HEAT:
            du.amount = guy.calculate_by_enchantment( du.amount, enchant_vals::mod::ARMOR_HEAT );
            break;
        case damage_type::STAB:
            du.amount = guy.calculate_by_enchantment( du.amount, enchant_vals::mod::ARMOR_STAB );
            break;
        case damage_type::BULLET:
            du.amount = guy.calculate_by_enchantment( du.amount, enchant_vals::mod::ARMOR_BULLET );
            break;
        default:
            return;
    }
    du.amount = std::max( 0.0f, du.amount );
}

void destroyed_armor_msg( Character &who, const std::string &pre_damage_name )
{
    if( who.is_avatar() ) {
        get_memorial().add(
            //~ %s is armor name
            pgettext( "memorial_male", "Worn %s was completely destroyed." ),
            pgettext( "memorial_female", "Worn %s was completely destroyed." ),
            pre_damage_name );
    }
    who.add_msg_player_or_npc( m_bad, _( "Your %s is completely destroyed!" ),
                               _( "<npcname>'s %s is completely destroyed!" ),
                               pre_damage_name );
}

void post_absorbed_damage_enchantment_adjust( Character &guy, damage_unit &du )
{
    switch( du.type ) {
        case damage_type::ACID:
            du.amount = guy.calculate_by_enchantment( du.amount, enchant_vals::mod::EXTRA_ACID );
            break;
        case damage_type::BASH:
            du.amount = guy.calculate_by_enchantment( du.amount, enchant_vals::mod::EXTRA_BASH );
            break;
        case damage_type::BIOLOGICAL:
            du.amount = guy.calculate_by_enchantment( du.amount, enchant_vals::mod::EXTRA_BIO );
            break;
        case damage_type::COLD:
            du.amount = guy.calculate_by_enchantment( du.amount, enchant_vals::mod::EXTRA_COLD );
            break;
        case damage_type::CUT:
            du.amount = guy.calculate_by_enchantment( du.amount, enchant_vals::mod::EXTRA_CUT );
            break;
        case damage_type::ELECTRIC:
            du.amount = guy.calculate_by_enchantment( du.amount, enchant_vals::mod::EXTRA_ELEC );
            break;
        case damage_type::HEAT:
            du.amount = guy.calculate_by_enchantment( du.amount, enchant_vals::mod::EXTRA_HEAT );
            break;
        case damage_type::STAB:
            du.amount = guy.calculate_by_enchantment( du.amount, enchant_vals::mod::EXTRA_STAB );
            break;
        case damage_type::BULLET:
            du.amount = guy.calculate_by_enchantment( du.amount, enchant_vals::mod::EXTRA_BULLET );
            break;
        default:
            return;
    }
    du.amount = std::max( 0.0f, du.amount );
}

const weakpoint *Character::absorb_hit( const weakpoint_attack &, const bodypart_id &bp,
                                        damage_instance &dam )
{
    std::list<item> worn_remains;
    bool armor_destroyed = false;

    for( damage_unit &elem : dam.damage_units ) {
        if( elem.amount < 0 ) {
            // Prevents 0 damage hits (like from hallucinations) from ripping armor
            elem.amount = 0;
            continue;
        }

        // The bio_ads CBM absorbs damage before hitting armor
        if( has_active_bionic( bio_ads ) ) {
            if( elem.amount > 0 && get_power_level() > 24_kJ ) {
                if( elem.type == damage_type::BASH ) {
                    elem.amount -= rng( 1, 2 );
                } else if( elem.type == damage_type::CUT ) {
                    elem.amount -= rng( 1, 4 );
                } else if( elem.type == damage_type::STAB || elem.type == damage_type::BULLET ) {
                    elem.amount -= rng( 1, 8 );
                }
                mod_power_level( -bio_ads->power_trigger );
            }
            if( elem.amount < 0 ) {
                elem.amount = 0;
            }
        }

        armor_enchantment_adjust( *this, elem );

        worn.absorb_damage( *this, elem, bp, worn_remains, armor_destroyed );

        passive_absorb_hit( bp, elem );

        post_absorbed_damage_enchantment_adjust( *this, elem );
        elem.amount = std::max( elem.amount, 0.0f );
    }
    map &here = get_map();
    for( item &remain : worn_remains ) {
        here.add_item_or_charges( pos(), remain );
    }
    if( armor_destroyed ) {
        drop_invalid_inventory();
    }
    return nullptr;
}

bool Character::armor_absorb( damage_unit &du, item &armor, const bodypart_id &bp,
                              const sub_bodypart_id &sbp, int roll )
{
    item::cover_type ctype = item::get_cover_type( du.type );

    // if the armor location has ablative armor apply that first
    if( armor.is_ablative() ) {
        ablative_armor_absorb( du, armor, sbp, roll );
    }

    // if the core armor is missed then exit
    if( roll > armor.get_coverage( sbp, ctype ) ) {
        return false;
    }

    // reduce the damage
    // -1 is passed as roll so that each material is rolled individually
    armor.mitigate_damage( du, sbp, -1 );

    // check if the armor was damaged
    item::armor_status damaged = armor.damage_armor_durability( du, bp );

    // describe what happened if the armor took damage
    if( damaged == item::armor_status::DAMAGED || damaged == item::armor_status::DESTROYED ) {
        describe_damage( du, armor );
    }
    return damaged == item::armor_status::DESTROYED;
}

bool Character::armor_absorb( damage_unit &du, item &armor, const bodypart_id &bp, int roll ) const
{
    item::cover_type ctype = item::get_cover_type( du.type );

    if( roll > armor.get_coverage( bp, ctype ) ) {
        return false;
    }

    // reduce the damage
    // -1 is passed as roll so that each material is rolled individually
    armor.mitigate_damage( du, bp, -1 );

    // check if the armor was damaged
    item::armor_status damaged = armor.damage_armor_durability( du, bp );

    // describe what happened if the armor took damage
    if( damaged == item::armor_status::DAMAGED || damaged == item::armor_status::DESTROYED ) {
        describe_damage( du, armor );
    }
    return damaged == item::armor_status::DESTROYED;
}

bool Character::ablative_armor_absorb( damage_unit &du, item &armor, const sub_bodypart_id &bp,
                                       int roll )
{
    item::cover_type ctype = item::get_cover_type( du.type );

    for( item_pocket *const pocket : armor.get_all_contained_pockets() ) {
        // if the pocket is ablative and not empty we should use its values
        if( pocket->get_pocket_data()->ablative && !pocket->empty() ) {
            // get the contained plate
            item &ablative_armor = pocket->front();

            float coverage = ablative_armor.get_coverage( bp, ctype );

            // if the attack hits this plate
            if( roll < coverage ) {
                damage_unit pre_mitigation = du;

                // mitigate the actual damage instance
                ablative_armor.mitigate_damage( du );

                // check if the item will break
                item::armor_status damaged = item::armor_status::UNDAMAGED;
                if( ablative_armor.find_armor_data()->non_functional != itype_id() ) {
                    // if the item transforms on destruction damage it that way
                    // ablative armor is concerned with incoming damage not mitigated damage
                    damaged = ablative_armor.damage_armor_transforms( pre_mitigation );
                } else {
                    damaged = ablative_armor.damage_armor_durability( du, bp->parent );
                }



                if( damaged == item::armor_status::TRANSFORMED ) {
                    //the plate is broken
                    const std::string pre_damage_name = ablative_armor.tname();

                    // TODO: add balistic and shattering verbs for ablative materials instead of hard coded
                    std::string format_string = _( "Your %1$s is %2$s!" );
                    std::string damage_verb = "shattered";
                    add_msg_if_player( m_bad, format_string, pre_damage_name, damage_verb );

                    if( is_avatar() ) {
                        SCT.add( point( posx(), posy() ), direction::NORTH, remove_color_tags( pre_damage_name ), m_neutral,
                                 damage_verb,
                                 m_info );
                    }

                    // delete the now destroyed item
                    itype_id replacement = ablative_armor.find_armor_data()->non_functional;
                    remove_item( ablative_armor );

                    // replace it with its destroyed version
                    pocket->add( item( replacement ) );

                    return true;
                }

                if( damaged == item::armor_status::DAMAGED ) {
                    //the plate is damaged like normal armor
                    describe_damage( du, ablative_armor );

                    return false;
                }

                if( damaged == item::armor_status::DESTROYED ) {
                    //the plate is damaged like normal armor but also ends up destroyed
                    describe_damage( du, ablative_armor );
                    if( get_player_view().sees( *this ) ) {
                        SCT.add( point( posx(), posy() ), direction::NORTH, remove_color_tags( ablative_armor.tname() ),
                                 m_neutral, _( "destroyed" ), m_info );
                    }
                    destroyed_armor_msg( *this, ablative_armor.tname() );

                    remove_item( ablative_armor );

                    return true;
                }

                return false;
            } else {
                // reduce value and try for additional plates
                roll = roll - coverage;
            }
        }
    }
    return false;

}

void Character::describe_damage( damage_unit &du, item &armor ) const
{
    const material_type &material = armor.get_random_material();
    std::string damage_verb = ( du.type == damage_type::BASH ) ? material.bash_dmg_verb() :
                              material.cut_dmg_verb();

    const std::string pre_damage_name = armor.tname();
    const std::string pre_damage_adj = armor.get_base_material().dmg_adj( armor.damage_level() );

    // add "further" if the damage adjective and verb are the same
    std::string format_string = ( pre_damage_adj == damage_verb ) ?
                                //~ %1$s is your armor name, %2$s is a damage verb
                                _( "Your %1$s is %2$s further!" ) : _( "Your %1$s is %2$s!" );
    add_msg_if_player( m_bad, format_string, pre_damage_name, damage_verb );
    //item is damaged
    if( is_avatar() ) {
        SCT.add( point( posx(), posy() ), direction::NORTH, remove_color_tags( pre_damage_name ), m_neutral,
                 damage_verb,
                 m_info );
    }
}

