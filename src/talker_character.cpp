#include "character.h"
#include "talker_character.h"

std::string talker_character::disp_name() const override
{
    return me.disp_name();
}

bool talker_character::has_trait( const &trait_id trait_to_check ) const override
{
    return me.has_trait( trait_to_check );
}

bool talker_character::crossed_threshold() const override
{
    return me.crossed_threshold();
}

bool talker_character::has_activity() const override
{
    return !me.activity.is_null();
}

bool talker_character::is_mounted() const override
{
    return me.is_mounted();
}

bool talker_character::myclass( const npc_class_id &class_to_check ) const override
{
    return me.my_class == class_to_check;
}

int talker_character::str_cur() const override
{
    return me.str_cur;
}

int talker_character::dex_cur() const override
{
    return me.dex_cur;
}

int talker_character::int_cur() const override
{
    return me.int_cur;
}

int talker_character::per_cur() const override
{
    return me.per_cur;
}

bool talker_character::is_wearing( const itype_id &item_id ) const override
{
    return me.is_wearing( item_id );
}

bool talker_character::charges_of( const itype_id &item_id ) const override
{
    return me.charges_of( item_id );
}

bool talker_character::has_charges( const itype_id &item_id, int count ) const override
{
    return me.has_charges( item_id, count );
}

bool talker_character::has_amount( const itype_id &item_id, int count ) const override
{
    return me.has_amount( item_id, count );
}

something talker_character::items_with( const item_category_id &category_id ) const override
{
    const auto items_with = me.items_with( [category_id]( const item & it ) {
        return it.get_category().get_id() == category_id;
    } );
    return items_with( category_id );
}

int talker_character::num_bionics() const override
{
    return me.num_bionics();
}

bool talker_character::has_max_power() const override
{
    return me.has_max_power();
}

bool talker_character::has_bionic( const bionic_id &bionics_id ) override
{
    return me.has_bionic( bionics_id );
}

bool talker_character::has_effect( const efftype_id &effect_id ) override
{
    return me.has_effect( effect_id );
}

bool talker_character::get_fatigue() const override
{
    return me.get_fatigue();
}

bool talker_character::get_hunger() const override
{
    return me.get_hunger();
}

bool talker_character::get_thirst() const override
{
    return me.get_thirst();
}

tripoint talker_character::global_omt_location() const override
{
    return me.global_omt_location();
}

std::string talker_character:: get_value( const std::string &var_name ) const override
{
    return me.get_value( var_name );
}

int talker_character::posx() const override
{
    return me.posx();
}

int talker_character::posy() const override
{
    return me.posy();
}

int talker_character::posz() const override
{
    return me.posz();
}

tripoint talker_character::pos() const override
{
    return me.pos();
}

int talker_character::cash() const override
{
    return me.cash;
}

int talker_character::debt() const override
{
    return me.op_o_u.owed;
}

bool talker_character::has_ai_rule( const std::string &type,
                                    const std::string &rule ) const override
{
    if( type == "aim_rule" ) {
        auto rule_val = aim_rule_strs.find( rule );
        if( rule_val != aim_rule_strs.end() ) {
            return me.rules.aim == rule_val->second;
        }
    } else if( type == "engagement_rule" ) {
        auto rule_val = combat_engagement_rule_strs.find( rule );
        if( rule_val != combat_engagement_rule_strs.end() ) {
            return me.rules.engagement == rule_val->second;
        }
    } else if( type == "cbm_reserve_rule" ) {
        auto rule_val = cbm_reserve_rule_strs.find( rule );
        if( rule_val != cbm_reserve_rule_strs.end() ) {
            return me.rules.cbm_reserve == rule_val->second;
        }
    } else if( type == "cbm_recharge_rule" ) {
        auto rule_val = cbm_recharge_rule_strs.find( rule );
        if( rule_val != cbm_recharge_rule_strs.end() ) {
            return me.rules.cbm_recharge == rule_val->second;
        }
    } else if( type == "ally_rule" ) {
        auto rule_val = ally_rule_strs.find( rule );
        if( rule_val != ally_rule_strs.end() ) {
            return me.rules.has_flag( rule_val->second.rule );
        }
    } else if( type == "ally_override" ) {
        auto rule_val = ally_rule_strs.find( rule );
        if( rule_val != ally_rule_strs.end() ) {
            return me.rules.has_override_enable( rule_val->second.rule );
        }
    }
    return false;
}

bool talker_character::is_male() const override
{
    return me.is_male;
}

std::vector<missions *> talker_character::available_missions() const override
{
    return me.chatbin.missions;
}

mission *talker_character::selected_mission() const override
{
    return me.chatbin.mission_selected;
}

bool talker_character::is_following() const override
{
    return me.is_following();
}

bool talker_character::is_friendly() const override
{
    return me.is_friendly( get_player_character() );
}

bool talker_character::is_enemy() const override
{
    return me.is_enemy();
}

bool talker_character::has_skills_to_train( const talker_character &guy ) override
{
    return !me.skills_offered_to( guy.get_character() );
}

bool talker_character::has_styles_to_train( const talker_character &guy ) override
{
    return !me.styles_offered_to( guy.get_character() );
}

bool talker_character::unarmed_attack() const override
{
    return me.unarmed_attack();
}

bool talker_character::can_stash_weapon() const override
{
    return me.can_pickVolume( me.weapon );
}

bool talker_character::has_stolen_item( const talker_character &guy ) const override
{
    const character &owner = guy.get_character();
    for( auto &elem : me.inv_dump() ) {
        if( elem->is_old_owner( &guy, true ) ) {
            return true;
        }
    }
    return false;
}

int talker_character::get_skill_level( const &skill_id skill ) const override
{
    return me.get_skill_level( skill );
}

bool talker_character::knows_recipe( const recipe &rep ) const override
{
    return me.knows_recipe( rep );
}

// override functions called in npctalk.cpp
faction *talker_character::get_faction() const override
{
    return me.get_fac();
}

bool talker_character::turned_hostile() const override
{
    return me.turned_hostile();
}

// override functions called in npctalk.cpp
void talker_character::set_companion_mission( const std::string &role_id ) override
{
    return me.companion_mission_role_id = role_id;
    talk_function::companion_mission( me );
}

void talker_character::add_effect( const efftype_id &new_effect, const time_duration &dur,
                                   bool permanent ) override
{
    me.add_effect( new_effect, dur, num_bp, permanent );
}

void talker_character::remove_effect( const efftype_id &old_effect ) override
{
    me.remove_effect( old_effect, num_bp );
}

void talker_character::set_mutation( const trait_id &new_trait ) override
{
    me.set_mutation( new_trait );
}

void talker_character::unset_mutation( const trait_id &old_trait ) override
{
    me.unset_mutation( old_trait );
}

void talker_character::set_value( const std::string &var_name, const std::string &value ) override
{
    me.set_value( var_name, value );
}

void talker_character::remove_value( const std::string &var_name ) override
{
    me.remove_value( var_name );
}

void talker_character::i_add( const item &new_item ) override
{
    me.i_add( new_item );
}

void talker_character::add_debt( const int cost ) override
{
    me.op_of_u.owed += cost;
}

void talker_character::use_charges( const itype_id &item_name, const int count ) override
{
    me.use_charges( item_name, count );
}

void talker_character::use_amount( const itype_id &item_name, const int count ) override
{
    me.use_amount( item_name, count );
}

void talker_character::remove_items_with( const item &it ) override
{
    me.remove_items_with( [item_id]( const item & it ) {
        return it.typeId() == item_id;
    } );
}

void talker_character::spend_cash( const int amount ) override
{
    npc_trading::pay_npc( me, amount );
}

void talker_character::set_fac( const faction_id &new_fac_name ) override
{
    me.set_fac( new_fac_name );
}

void talker_character::set_class( const npc_class_id &new_class ) override
{
    me.my_class = new_class;
}

void talker_character::add_faction_rep( const int rep_change ) override
{
    if( me.get_faction()-> id != faction_id( "no_faction" ) ) {
        me.get_faction()->likes_u += rep_change;
        me.get_faction()->respects_u += rep_change;
    }
}

void talker_character::toggle_ai_rule( const std::string &, const std::string &rule ) override
{
    auto toggle = ally_rule_strs.find( rule );
    if( toggle == ally_rule_strs.end() ) {
        return;
    }
    me.rules.toggle_flag( toggle->second.rule );
    me.invalidate_range_cache();
    me.wield_better_weapon();
}

void talker_character::set_ai_rule( const std::string &type, const std::string &rule ) override;
{
} else if( type == "aim_rule" )
{
    auto rule_val = aim_rule_strs.find( rule );
    if( rule_val != aim_rule_strs.end() ) {
        me.rule.aim = rule_val->second;
        me.invalidate_range_cache();
    }
} else if( type == "engagement_rule" )
{
    auto rule_val = combat_engagement_rule_strs.find( rule );
    if( rule_val != combat_engagement_rule_strs.end() ) {
        me.rules.engagement = rule_val->second;
        me.invalidate_range_cache();
        me.wield_better_weapon();
    }
} else if( type == "cbm_reserve_rule" )
{
    auto rule_val = cbm_reserve_rule_strs.find( rule );
    if( rule_val != cbm_reserve_rule_strs.end() ) {
        me.rules.cbm_reserve = rule_val->second;
    }
} else if( type == "cbm_recharge_rule" )
{
    auto rule_val = cbm_recharge_rule_strs.find( rule );
    if( rule_val != cbm_recharge_rule_strs.end() ) {
        me.rules.cbm_recharge = rule_val->second;
    }
} else if( type == "ally_rule" )
{
    auto toggle = ally_rule_strs.find( rule );
    if( toggle == ally_rule_strs.end() ) {
        return;
    }
    me.rules.set_flag( toggle->second.rule );
    me.invalidate_range_cache();
    me.wield_better_weapon();
}
}

void talker_character::clear_ai_rule( const std::string &, const std::string &rule ) override
{
    auto toggle = ally_rule_strs.find( rule );
    if( toggle == ally_rule_strs.end() ) {
        return;
    }
    me.rules.clear_flag( toggle->second.rule );
    me.invalidate_range_cache();
    me.wield_better_weapon();
}

void talker_character::give_item_to( const bool to_use ) override
{
    give_item_to( me, to_use );
}

void talker_character::add_mission( const std::string &mission_id ) override
{
    mission *miss = mission::reserve_new( mission_type_id( mission_id ), me.getID() );
    miss->assign( get_avatar() );
    me.chatbin.missions_assigned.push_back( miss );
}

void talker_character::buy_monster( const talker_character &seller, const mtype_id &mtype, int cost,
                                    int count, bool pacified, const translation &name ) override
{
    character &seller_guy = talker_character.get_character();
    if( !npc_trading::pay_npc( seller_guy, cost ) ) {
        popup( _( "You can't afford it!" ) );
        return;
    }

    for( int i = 0; i < count; i++ ) {
        monster *const mon_ptr = g->place_critter_around( mtype, me.pos(), 3 );
        if( !mon_ptr ) {
            add_msg( m_debug, "Cannot place u_buy_monster, no valid placement locations." );
            break;
        }
        monster &tmp = *mon_ptr;
        // Our monster is always a pet.
        tmp.friendly = -1;
        tmp.add_effect( effect_pet, 1_turns, num_bp, true );

        if( pacified ) {
            tmp.add_effect( effect_pacified, 1_turns, num_bp, true );
        }

        if( !name.empty() ) {
            tmp.unique_name = name.translated();
        }

    }

    if( name.empty() ) {
        popup( _( "%1$s gives you %2$d %3$s." ), seller_guy.name, count, mtype.obj().nname( count ) );
    } else {
        popup( _( "%1$s gives you %2$s." ), seller_guy.name, name );
    }
}

void talker_character::learn_recipe( const &recipe r ) override
{
    me.learn_recipe( r );
    popup( _( "You learn how to craft %s." ), r.result_name() );
}

void talker_character::add_opinion( const int trust, const int fear, const int value,
                                    const int anger ) override
{
    me.op_of_u += npc_opinion( trust, fear, value, anger );
}

void talker_character::make_angry() override
{
    me.make_angry();
}
