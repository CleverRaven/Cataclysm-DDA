#include <algorithm>
#include <list>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "bionics.h"
#include "bodypart.h"
#include "character.h"
#include "character_attire.h"
#include "damage.h"
#include "enums.h"
#include "flag.h"
#include "item.h"
#include "item_pocket.h"
#include "itype.h"
#include "line.h"
#include "magic_enchantment.h"
#include "make_static.h"
#include "map.h"
#include "material.h"
#include "memorial_logger.h"
#include "mutation.h"
#include "npc.h"
#include "output.h"
#include "pimpl.h"
#include "point.h"
#include "rng.h"
#include "subbodypart.h"
#include "translation.h"
#include "translations.h"
#include "type_id.h"
#include "units.h"
#include "viewer.h"

struct weakpoint;
struct weakpoint_attack;

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

float Character::mutation_armor( bodypart_id bp, const damage_type_id &dt ) const
{
    return mutation_armor( bp ).type_resist( dt );
}

float Character::mutation_armor( bodypart_id bp, const damage_unit &du ) const
{
    return mutation_armor( bp ).get_effective_resist( du );
}

int Character::get_armor_type( const damage_type_id &dt, bodypart_id bp ) const
{
    if( dt->no_resist ) {
        return 0;
    }
    int ret = worn.damage_resist( dt, bp );
    auto bonus = armor_bonus.find( dt );
    if( bonus != armor_bonus.end() ) {
        ret += bonus->second;
    }
    for( const bionic_id &bid : get_bionics() ) {
        const auto prot = bid->protec.find( bp.id() );
        if( prot != bid->protec.end() ) {
            ret += prot->second.type_resist( dt );
        }
    }
    return ret + mutation_armor( bp, dt ) + bp->damage_resistance( dt );
}

std::map<bodypart_id, int> Character::get_all_armor_type( const damage_type_id &dt,
        const std::map<bodypart_id, std::vector<const item *>> &clothing_map ) const
{
    std::map<bodypart_id, int> ret;
    for( const bodypart_id &bp : get_all_body_parts() ) {
        ret.emplace( bp, get_armor_type( dt, bp ) );
        for( const item *it : clothing_map.at( bp ) ) {
            ret[bp] += it->resist( dt, false, bp );
        }
    }
    return ret;
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

// adjusts damage unit depending on type by enchantments.
// the ITEM_ enchantments only affect the damage resistance for that one item, while the others affect all of them
static void armor_enchantment_adjust( Character &guy, damage_unit &du )
{
    //If we're not dealing any damage of the given type, don't even bother.
    if( du.amount < 0.1f ) {
        return;
    }
    // FIXME: hardcoded damage types -> enchantments
    if( du.type == STATIC( damage_type_id( "acid" ) ) ) {
        du.amount = guy.calculate_by_enchantment( du.amount, enchant_vals::mod::ARMOR_ACID );
    } else if( du.type == STATIC( damage_type_id( "bash" ) ) ) {
        du.amount = guy.calculate_by_enchantment( du.amount, enchant_vals::mod::ARMOR_BASH );
    } else if( du.type == STATIC( damage_type_id( "biological" ) ) ) {
        du.amount = guy.calculate_by_enchantment( du.amount, enchant_vals::mod::ARMOR_BIO );
    } else if( du.type == STATIC( damage_type_id( "cold" ) ) ) {
        du.amount = guy.calculate_by_enchantment( du.amount, enchant_vals::mod::ARMOR_COLD );
    } else if( du.type == STATIC( damage_type_id( "cut" ) ) ) {
        du.amount = guy.calculate_by_enchantment( du.amount, enchant_vals::mod::ARMOR_CUT );
    } else if( du.type == STATIC( damage_type_id( "electric" ) ) ) {
        du.amount = guy.calculate_by_enchantment( du.amount, enchant_vals::mod::ARMOR_ELEC );
    } else if( du.type == STATIC( damage_type_id( "heat" ) ) ) {
        du.amount = guy.calculate_by_enchantment( du.amount, enchant_vals::mod::ARMOR_HEAT );
    } else if( du.type == STATIC( damage_type_id( "stab" ) ) ) {
        du.amount = guy.calculate_by_enchantment( du.amount, enchant_vals::mod::ARMOR_STAB );
    } else if( du.type == STATIC( damage_type_id( "bullet" ) ) ) {
        du.amount = guy.calculate_by_enchantment( du.amount, enchant_vals::mod::ARMOR_BULLET );
    }
    if( du.type != STATIC( damage_type_id( "pure" ) ) ) {
        du.amount = guy.calculate_by_enchantment( du.amount, enchant_vals::mod::ARMOR_ALL );
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
    // FIXME: hardcoded damage types -> enchantments
    if( du.type == STATIC( damage_type_id( "acid" ) ) ) {
        du.amount = guy.calculate_by_enchantment( du.amount, enchant_vals::mod::EXTRA_ACID );
    } else if( du.type == STATIC( damage_type_id( "bash" ) ) ) {
        du.amount = guy.calculate_by_enchantment( du.amount, enchant_vals::mod::EXTRA_BASH );
    } else if( du.type == STATIC( damage_type_id( "biological" ) ) ) {
        du.amount = guy.calculate_by_enchantment( du.amount, enchant_vals::mod::EXTRA_BIO );
    } else if( du.type == STATIC( damage_type_id( "cold" ) ) ) {
        du.amount = guy.calculate_by_enchantment( du.amount, enchant_vals::mod::EXTRA_COLD );
    } else if( du.type == STATIC( damage_type_id( "cut" ) ) ) {
        du.amount = guy.calculate_by_enchantment( du.amount, enchant_vals::mod::EXTRA_CUT );
    } else if( du.type == STATIC( damage_type_id( "electric" ) ) ) {
        du.amount = guy.calculate_by_enchantment( du.amount, enchant_vals::mod::EXTRA_ELEC );
    } else if( du.type == STATIC( damage_type_id( "heat" ) ) ) {
        du.amount = guy.calculate_by_enchantment( du.amount, enchant_vals::mod::EXTRA_HEAT );
    } else if( du.type == STATIC( damage_type_id( "stab" ) ) ) {
        du.amount = guy.calculate_by_enchantment( du.amount, enchant_vals::mod::EXTRA_STAB );
    } else if( du.type == STATIC( damage_type_id( "bullet" ) ) ) {
        du.amount = guy.calculate_by_enchantment( du.amount, enchant_vals::mod::EXTRA_BULLET );
    }
    du.amount = std::max( 0.0f, du.amount );
}

const weakpoint *Character::absorb_hit( const weakpoint_attack &, const bodypart_id &bp,
                                        damage_instance &dam )
{
    std::list<item> worn_remains;
    bool armor_destroyed = false;

    bool damage_mitigated = false;

    double forcefield = enchantment_cache->modify_value( enchant_vals::mod::FORCEFIELD, 0.0 );

    if( rng( 0, 99 ) < forcefield * 100.0 ) {
        add_msg_if_player( m_good,
                           _( "The incoming attack was made ineffective." ) );
        damage_mitigated = true;
    }

    for( damage_unit &elem : dam.damage_units ) {
        if( damage_mitigated ) {
            elem.amount = 0;
        }

        if( elem.amount < 0 ) {
            // Prevents 0 damage hits (like from hallucinations) from ripping armor
            elem.amount = 0;
            continue;
        }

        // The bio_ads CBM absorbs damage before hitting armor
        if( has_active_bionic( bio_ads ) ) {
            bool absorbed = false;
            if( elem.amount > 0 && get_power_level() > bio_ads->power_trigger ) {
                if( elem.type == STATIC( damage_type_id( "bash" ) ) ) {
                    elem.amount -= rng( 1, 4 );
                    absorbed = true;
                } else if( elem.type == STATIC( damage_type_id( "cut" ) ) ) {
                    elem.amount -= rng( 2, 8 );
                    absorbed = true;
                } else if( elem.type == STATIC( damage_type_id( "stab" ) ) ||
                           STATIC( damage_type_id( "bullet" ) ) ) {
                    elem.amount -= rng( 4, 16 );
                    absorbed = true;
                }
                if( absorbed ) {
                    mod_power_level( -bio_ads->power_trigger );
                    add_msg_if_player( m_good,
                                       _( "The defensive forcefield surrounding you ripples as it reduces the velocity of the incoming attack." ) );
                }
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
                              const sub_bodypart_id &sbp, int roll ) const
{
    item::cover_type ctype = item::get_cover_type( du.type );

    // if the core armor is missed then exit
    if( roll > armor.get_coverage( sbp, ctype ) ) {
        return false;
    }
    // if this armor has the flag, try to deduct that much energy from it. If that takes it to 0 energy, turn it off before it absorbs damage.
    if( armor.has_flag( flag_USE_POWER_WHEN_HIT ) &&
        units::from_kilojoule( du.amount ) > armor.energy_consume( units::from_kilojoule( du.amount ),
                pos(), nullptr ) ) {
        armor.deactivate( nullptr, false );
        add_msg_if_player( _( "Your %s doesn't have enough power and shuts down!" ), armor.tname() );
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
    // if this armor has the flag, try to deduct that much energy from it. If that takes it to 0 energy, turn it off before it absorbs damage.
    if( armor.has_flag( flag_USE_POWER_WHEN_HIT ) &&
        units::from_kilojoule( du.amount ) > armor.energy_consume( units::from_kilojoule( du.amount ),
                pos(), nullptr ) ) {
        armor.deactivate( nullptr, false );
        add_msg_if_player( _( "Your %s doesn't have enough power and shuts down!" ), armor.tname() );
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

    for( item_pocket *const pocket : armor.get_all_ablative_pockets() ) {
        // if the pocket is ablative and not empty we should use its values
        if( !pocket->empty() ) {
            // get the contained plate
            item &ablative_armor = pocket->front();

            float coverage = ablative_armor.get_coverage( bp, ctype );

            // if the attack hits this plate
            if( roll <= coverage ) {
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
                    std::string format_string = _( "Your %1$s %2$s!" );
                    std::string damage_verb = "is shattered";
                    if( !ablative_armor.find_armor_data()->damage_verb.empty() ) {
                        damage_verb = ablative_armor.find_armor_data()->damage_verb.translated();
                    }
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
    // FIXME: Hardcoded damage types
    std::string damage_verb = ( du.type == STATIC( damage_type_id( "bash" ) ) ) ?
                              material.bash_dmg_verb() : material.cut_dmg_verb();

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

