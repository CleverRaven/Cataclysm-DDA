#include "math_parser_diag.h"

#include <functional>
#include <string>
#include <vector>

#include "condition.h"
#include "dialogue.h"
#include "field.h"
#include "game.h"
#include "magic.h"
#include "math_parser_shim.h"
#include "mtype.h"
#include "options.h"
#include "string_input_popup.h"
#include "units.h"
#include "weather.h"

namespace
{
bool is_beta( char scope )
{
    switch( scope ) {
        case 'n':
            return true;
        case 'u':
        default:
            return false;
    }
}

} // namespace

std::function<double( dialogue & )> u_val( char scope,
        std::vector<diag_value> const &params, diag_kwargs const &/* kwargs */ )
{
    kwargs_shim const shim( params, scope );
    try {
        return conditional_t::get_get_dbl( shim );
    } catch( std::exception const &e ) {
        debugmsg( "shim failed: %s", e.what() );
        return []( dialogue const & ) {
            return 0;
        };
    }
}

std::function<void( dialogue &, double )> u_val_ass( char scope,
        std::vector<diag_value> const &params, diag_kwargs const &/* kwargs */ )
{
    kwargs_shim const shim( params, scope );
    try {
        return conditional_t::get_set_dbl( shim, {}, {}, false );
    } catch( std::exception const &e ) {
        debugmsg( "shim failed: %s", e.what() );
        return []( dialogue const &, double ) {};
    }
}

std::function<double( dialogue & )> option_eval( char /* scope */,
        std::vector<diag_value> const &params, diag_kwargs const &/* kwargs */ )
{
    return[option = params[0]]( dialogue const & d ) {
        return get_option<float>( option.str( d ) );
    };
}

std::function<double( dialogue & )> addiction_intensity_eval( char scope,
        std::vector<diag_value> const &params, diag_kwargs const &/* kwargs */ )
{
    return[ beta = is_beta( scope ), add_value = params[0]]( dialogue const & d ) {
        return d.actor( beta )->get_addiction_intensity( addiction_id( add_value.str( d ) ) );
    };
}

std::function<double( dialogue & )> addiction_turns_eval( char scope,
        std::vector<diag_value> const &params, diag_kwargs const &/* kwargs */ )
{
    return[ beta = is_beta( scope ), add_value = params[0]]( dialogue const & d ) {
        return d.actor( beta )->get_addiction_turns( addiction_id( add_value.str( d ) ) );
    };
}

std::function<void( dialogue &, double )> addiction_turns_ass( char scope,
        std::vector<diag_value> const &params, diag_kwargs const &/* kwargs */ )
{
    return[ beta = is_beta( scope ), add_value = params[0]]( dialogue const & d, double val ) {
        return d.actor( beta )->set_addiction_turns( addiction_id( add_value.str( d ) ), val );
    };
}

std::function<double( dialogue & )> armor_eval( char scope,
        std::vector<diag_value> const &params, diag_kwargs const &/* kwargs */ )
{
    return[type = params[0], bpid = params[1], beta = is_beta( scope )]( dialogue const & d ) {
        damage_type_id dt( type.str( d ) );
        bodypart_id bp( bpid.str( d ) );
        return d.actor( beta )->armor_at( dt, bp );
    };
}

std::function<double( dialogue & )> charge_count_eval( char scope,
        std::vector<diag_value> const &params, diag_kwargs const &/* kwargs */ )
{
    return[beta = is_beta( scope ), item_value = params[0]]( dialogue const & d ) {
        return d.actor( beta )->charges_of( itype_id( item_value.str( d ) ) );
    };
}

std::function<double( dialogue & )> coverage_eval( char scope,
        std::vector<diag_value> const &params, diag_kwargs const &/* kwargs */ )
{
    return[bpid = params[0], beta = is_beta( scope )]( dialogue const & d ) {
        bodypart_id bp( bpid.str( d ) );
        return d.actor( beta )->coverage_at( bp );
    };
}

std::function<double( dialogue & )> distance_eval( char scope,
        std::vector<diag_value> const &params, diag_kwargs const &/* kwargs */ )
{
    return[params, beta = is_beta( scope )]( dialogue const & d ) {
        const auto get_pos = [&d]( std::string_view str ) {
            if( str == "u" ) {
                return d.actor( false )->global_pos();
            } else if( str == "npc" ) {
                return d.actor( true )->global_pos();
            }
            return tripoint_abs_ms( tripoint::from_string( str.data() ) );
        };
        return rl_dist( get_pos( params[0].str( d ) ), get_pos( params[1].str( d ) ) );
    };
}

std::function<double( dialogue & )> effect_intensity_eval( char scope,
        std::vector<diag_value> const &params, diag_kwargs const &kwargs )
{
    diag_value bp_val( std::string{} );
    if( kwargs.count( "bodypart" ) != 0 ) {
        bp_val = *kwargs.at( "bodypart" );
    }
    return[effect_id = params[0], bp_val, beta = is_beta( scope )]( dialogue const & d ) {
        std::string const bp_str = bp_val.str( d );
        bodypart_id const bp = bp_str.empty() ? bodypart_str_id::NULL_ID() : bodypart_id( bp_str );
        effect target = d.actor( beta )->get_effect( efftype_id( effect_id.str( d ) ), bp );
        return target.is_null() ? -1 : target.get_intensity();
    };
}

std::function<double( dialogue & )> encumbrance_eval( char scope,
        std::vector<diag_value> const &params, diag_kwargs const &/* kwargs */ )
{
    return[bpid = params[0], beta = is_beta( scope )]( dialogue const & d ) {
        bodypart_id bp( bpid.str( d ) );
        return d.actor( beta )->encumbrance_at( bp );
    };
}

std::function<double( dialogue & )> field_strength_eval( char scope,
        std::vector<diag_value> const &params, diag_kwargs const &kwargs )
{
    std::optional<var_info> loc_var;
    if( kwargs.count( "location" ) != 0 ) {
        loc_var = kwargs.at( "location" )->var();
    } else if( scope == 'g' ) {
        throw std::invalid_argument( string_format(
                                         R"("field_strength" needs either an actor scope (u/n) or a 'location' kwarg)" ) );
    }

    return [beta = is_beta( scope ), field_value = params[0], loc_var]( dialogue & d ) {
        map &here = get_map();
        tripoint_abs_ms loc;
        if( loc_var.has_value() ) {
            loc = get_tripoint_from_var( loc_var, d );
        } else {
            loc = d.actor( beta )->global_pos();
        }
        field_type_id ft = field_type_id( field_value.str( d ) );
        field_entry *fp = here.field_at( here.getlocal( loc ) ).find_field( ft );
        return fp ? fp->get_field_intensity() :  0;
    };
}

std::function<double( dialogue & )> has_trait_eval( char scope,
        std::vector<diag_value> const &params, diag_kwargs const &/* kwargs */ )
{
    return [beta = is_beta( scope ), tid = params[0] ]( dialogue const & d ) {
        return d.actor( beta )->has_trait( trait_id( tid.str( d ) ) );
    };
}

std::function<double( dialogue & )> knows_proficiency_eval( char scope,
        std::vector<diag_value> const &params, diag_kwargs const &/* kwargs */ )
{
    return [beta = is_beta( scope ), tid = params[0] ]( dialogue const & d ) {
        return d.actor( beta )->knows_proficiency( proficiency_id( tid.str( d ) ) );
    };
}

std::function<double( dialogue & )> hp_eval( char scope,
        std::vector<diag_value> const &params, diag_kwargs const &/* kwargs */ )
{
    return[bp_val = params[0], beta = is_beta( scope )]( dialogue const & d ) {
        std::string const bp_str = bp_val.str( d );
        bodypart_id const bp = bp_str == "ALL" ? bodypart_str_id::NULL_ID() : bodypart_id( bp_str );
        return d.actor( beta )->get_cur_hp( bp );
    };
}

std::function<void( dialogue &, double )> hp_ass( char scope,
        std::vector<diag_value> const &params, diag_kwargs const &/* kwargs */ )
{
    return [bp_val = params[0], beta = is_beta( scope )]( dialogue const & d, double val ) {
        std::string const bp_str = bp_val.str( d );
        if( bp_str == "ALL" ) {
            d.actor( beta )->set_all_parts_hp_cur( val );
        } else {
            d.actor( beta )->set_part_hp_cur( bodypart_id( bp_str ), val );
        }
    };
}

std::function<void( dialogue &, double )> spellcasting_adjustment_ass( char scope,
        std::vector<diag_value> const &params, diag_kwargs const &kwargs )
{
    enum spell_scope {
        scope_all,
        scope_mod,
        scope_school,
        scope_spell
    };
    diag_value filter( std::string{} );
    spell_scope spellsearch_scope;
    if( kwargs.count( "mod" ) != 0 ) {
        filter = *kwargs.at( "mod" );
        spellsearch_scope = scope_mod;
    } else if( kwargs.count( "school" ) != 0 ) {
        filter = *kwargs.at( "school" );
        spellsearch_scope = scope_school;
    } else if( kwargs.count( "spell" ) != 0 ) {
        filter = *kwargs.at( "spell" );
        spellsearch_scope = scope_spell;
    } else {
        spellsearch_scope = scope_all;
    }

    diag_value whitelist( std::string{} );
    diag_value blacklist( std::string{} );
    if( kwargs.count( "flag_whitelist" ) != 0 ) {
        whitelist = *kwargs.at( "flag_whitelist" );
    }
    if( kwargs.count( "flag_blacklist" ) != 0 ) {
        blacklist = *kwargs.at( "flag_blacklist" );
    }

    return[beta = is_beta( scope ),
                spellcasting_property = params[0], whitelist, blacklist, spellsearch_scope,
         filter]( dialogue const & d, double val ) {
        std::string const filter_str = filter.str( d );
        switch( spellsearch_scope ) {
            case scope_spell:
                d.actor( beta )->get_character()->magic->get_spell( spell_id( filter_str ) ).set_temp_adjustment(
                    spellcasting_property.str( d ), val );
                break;
            case scope_school: {
                const trait_id school_id( filter_str );
                for( spell *spellIt : d.actor( beta )->get_character()->magic->get_spells() ) {
                    if( spellIt->spell_class() == school_id
                        && ( whitelist.str( d ).empty() || spellIt->has_flag( whitelist.str( d ) ) )
                        && ( blacklist.str( d ).empty() || !spellIt->has_flag( blacklist.str( d ) ) )
                      ) {
                        spellIt->set_temp_adjustment( spellcasting_property.str( d ), val );
                    }
                }
                break;
            }
            case scope_mod: {
                const mod_id target_mod_id( filter_str );
                for( spell *spellIt : d.actor( beta )->get_character()->magic->get_spells() ) {
                    if( spellIt->get_src() == target_mod_id
                        && ( whitelist.str( d ).empty() || spellIt->has_flag( whitelist.str( d ) ) )
                        && ( blacklist.str( d ).empty() || !spellIt->has_flag( blacklist.str( d ) ) )
                      ) {
                        spellIt->set_temp_adjustment( spellcasting_property.str( d ), val );
                    }
                }
                break;
            }
            case scope_all:
                for( spell *spellIt : d.actor( beta )->get_character()->magic->get_spells() ) {
                    if( ( whitelist.str( d ).empty() || spellIt->has_flag( whitelist.str( d ) ) )
                        && ( blacklist.str( d ).empty() || !spellIt->has_flag( blacklist.str( d ) ) )
                      ) {
                        spellIt->set_temp_adjustment( spellcasting_property.str( d ), val );
                    }
                }
                break;
        }
    };
}

std::function<double( dialogue & )> hp_max_eval( char scope,
        std::vector<diag_value> const &params, diag_kwargs const &/* kwargs */ )
{
    return[bpid = params[0], beta = is_beta( scope )]( dialogue const & d ) {
        bodypart_id bp( bpid.str( d ) );
        return d.actor( beta )->get_hp_max( bp );
    };
}

std::function<double( dialogue & )> item_count_eval( char scope,
        std::vector<diag_value> const &params, diag_kwargs const &/* kwargs */ )
{
    return[beta = is_beta( scope ), item_value = params[0]]( dialogue const & d ) {
        return d.actor( beta )->get_amount( itype_id( item_value.str( d ) ) );
    };
}

std::function<double( dialogue & )> num_input_eval( char /*scope*/,
        std::vector<diag_value> const &params, diag_kwargs const &/* kwargs */ )
{
    return[prompt = params[0], default_val = params[1]]( dialogue const & d ) {
        string_input_popup popup;
        double dv = default_val.dbl( d );
        int popup_val = dv;
        popup
        .title( _( "Input a value:" ) )
        .width( 20 )
        .description( prompt.str( d ) )
        .edit( popup_val );
        if( popup.canceled() ) {
            return dv;
        }
        return static_cast<double>( popup_val );
    };
}

std::function<double( dialogue & )> attack_speed_eval( char scope,
        std::vector<diag_value> const &/* params */, diag_kwargs const &/* kwargs */ )
{
    return[beta = is_beta( scope )]( dialogue const & d ) {
        return d.actor( beta )->attack_speed();
    };
}

namespace
{
bool _filter_monster( Creature const &critter, std::vector<mtype_id> const &ids, int radius,
                      tripoint_abs_ms const &loc )
{
    if( critter.is_monster() ) {
        mtype_id const mid = critter.as_monster()->type->id;
        bool const id_filter =
        ids.empty() || std::any_of( ids.begin(), ids.end(), [&mid]( mtype_id const & id ) {
            return id == mid;
        } );
        // friendly to the player, not a target for us
        return id_filter && critter.as_monster()->friendly == 0 &&
               radius >= rl_dist( critter.get_location(), loc );
    }
    return false;
}

} // namespace

std::function<double( dialogue & )> monsters_nearby_eval( char scope,
        std::vector<diag_value> const &params, diag_kwargs const &kwargs )
{
    diag_value radius_val( 1000.0 );
    std::optional<var_info> loc_var;
    if( kwargs.count( "radius" ) != 0 ) {
        radius_val = *kwargs.at( "radius" );
    }
    if( kwargs.count( "location" ) != 0 ) {
        loc_var = kwargs.at( "location" )->var();
    } else if( scope == 'g' ) {
        throw std::invalid_argument( string_format(
                                         R"("monsters_nearby" needs either an actor scope (u/n) or a 'location' kwarg)" ) );
    }

    return [beta = is_beta( scope ), params, loc_var, radius_val]( dialogue & d ) {
        tripoint_abs_ms loc;
        if( loc_var.has_value() ) {
            loc = get_tripoint_from_var( loc_var, d );
        } else {
            loc = d.actor( beta )->global_pos();
        }

        int const radius = static_cast<int>( radius_val.dbl( d ) );
        std::vector<mtype_id> mids( params.size() );
        std::transform( params.begin(), params.end(), mids.begin(), [&d]( diag_value const & val ) {
            return mtype_id( val.str( d ) );
        } );

        std::vector<Creature *> const targets = g->get_creatures_if( [&mids, &radius,
               &loc]( const Creature & critter ) {
            return _filter_monster( critter, mids, radius, loc );
        } );
        return static_cast<double>( targets.size() );
    };
}

std::function<double( dialogue & )> pain_eval( char scope,
        std::vector<diag_value> const &/* params */, diag_kwargs const &/* kwargs */ )
{
    return [beta = is_beta( scope )]( dialogue const & d ) {
        return d.actor( beta )->pain_cur();
    };
}

std::function<void( dialogue &, double )> pain_ass( char scope,
        std::vector<diag_value> const &/* params */, diag_kwargs const &/* kwargs */ )
{
    return [beta = is_beta( scope )]( dialogue const & d, double val ) {
        d.actor( beta )->set_pain( val );
    };
}

std::function<double( dialogue & )> school_level_eval( char scope,
        std::vector<diag_value> const &params, diag_kwargs const &/* kwargs */ )
{
    return[beta = is_beta( scope ), school_value = params[0]]( dialogue const & d ) {
        return d.actor( beta )->get_spell_level( trait_id( school_value.str( d ) ) );
    };
}

std::function<double( dialogue & )> school_level_adjustment_eval( char scope,
        std::vector<diag_value> const &params, diag_kwargs const &/* kwargs */ )
{
    return[beta = is_beta( scope ), school_value = params[0]]( dialogue const & d ) {
        const Character *ch = d.actor( beta )->get_character();
        if( ch ) {
            const trait_id school( school_value.str( d ) );
            auto it = ch->magic->caster_level_adjustment_by_school.find( school );
            if( it != ch->magic->caster_level_adjustment_by_school.end() ) {
                return it->second;
            } else {
                return 0.0;
            }
        }
        return 0.0;
    };
}

std::function<void( dialogue &, double )> school_level_adjustment_ass( char scope,
        std::vector<diag_value> const &params, diag_kwargs const &/* kwargs */ )
{
    return[beta = is_beta( scope ), school_value = params[0]]( dialogue const & d, double val ) {
        const Character *ch = d.actor( beta )->get_character();
        if( ch ) {
            const trait_id school( school_value.str( d ) );
            auto it = ch->magic->caster_level_adjustment_by_school.find( school );
            if( it != ch->magic->caster_level_adjustment_by_school.end() ) {
                it->second = val;
            } else {
                ch->magic->caster_level_adjustment_by_school.insert( { school, val } );
            }
        }
        return 0.0;
    };
}

std::function<double( dialogue & )> skill_eval( char scope,
        std::vector<diag_value> const &params, diag_kwargs const &/* kwargs */ )
{
    return [beta = is_beta( scope ), sid = params[0] ]( dialogue const & d ) {
        return d.actor( beta )->get_skill_level( skill_id( sid.str( d ) ) );
    };
}

std::function<void( dialogue &, double )> skill_ass( char scope,
        std::vector<diag_value> const &params, diag_kwargs const &/* kwargs */ )
{
    return [beta = is_beta( scope ), sid = params[0] ]( dialogue const & d, double val ) {
        return d.actor( beta )->set_skill_level( skill_id( sid.str( d ) ), val );
    };
}

std::function<double( dialogue & )> skill_exp_eval( char scope,
        std::vector<diag_value> const &params, diag_kwargs const &kwargs )
{
    diag_value format_value( std::string( "percentage" ) );
    if( kwargs.count( "format" ) != 0 ) {
        format_value = *kwargs.at( "format" );
    }

    return[skill_value = params[0], format_value, beta = is_beta( scope )]( dialogue const & d ) {
        skill_id skill( skill_value.str( d ) );
        std::string format = format_value.str( d );
        if( format != "raw" && format != "percentage" ) {
            debugmsg( R"(Unknown format type "%s" for skill_exp, assumning "percentage")", format );
        }
        bool raw = format == "raw";
        return d.actor( beta )->get_skill_exp( skill, raw );
    };
}

std::function<void( dialogue &, double )> skill_exp_ass( char scope,
        std::vector<diag_value> const &params, diag_kwargs const &kwargs )
{
    diag_value format_value( std::string( "percentage" ) );
    if( kwargs.count( "format" ) != 0 ) {
        format_value = *kwargs.at( "format" );
    }

    return [skill_value = params[0], format_value, beta = is_beta( scope ) ]( dialogue const & d,
    double val ) {
        skill_id skill( skill_value.str( d ) );
        std::string format = format_value.str( d );
        if( format != "raw" && format != "percentage" ) {
            debugmsg( R"(Unknown format type "%s" for skill_exp, assumning "percentage")", format );
        }
        bool raw = format == "raw";
        return d.actor( beta )->set_skill_exp( skill, val, raw );
    };
}

std::function<double( dialogue & )> spell_count_eval( char scope,
        std::vector<diag_value> const &/* params */, diag_kwargs const &kwargs )
{
    diag_value school_value( std::string{} );
    if( kwargs.count( "school" ) != 0 ) {
        school_value = *kwargs.at( "school" );
    }
    return[beta = is_beta( scope ), school_value]( dialogue const & d ) {
        std::string school_str = school_value.str( d );
        const trait_id scid = school_str.empty() ? trait_id::NULL_ID() : trait_id( school_str );
        return d.actor( beta )->get_spell_count( scid );
    };
}

std::function<double( dialogue & )> spell_exp_eval( char scope,
        std::vector<diag_value> const &params, diag_kwargs const &/* kwargs */ )
{
    return[beta = is_beta( scope ), sid = params[0]]( dialogue const & d ) {
        return d.actor( beta )->get_spell_exp( spell_id( sid.str( d ) ) );
    };
}

std::function<void( dialogue &, double )> spell_exp_ass( char scope,
        std::vector<diag_value> const &params, diag_kwargs const &/* kwargs */ )
{
    return[beta = is_beta( scope ), sid = params[0]]( dialogue const & d, double val ) {
        return d.actor( beta )->set_spell_exp( spell_id( sid.str( d ) ), val );
    };
}

std::function<double( dialogue & )> spell_level_eval( char scope,
        std::vector<diag_value> const &params, diag_kwargs const &/* kwargs */ )
{
    return[beta = is_beta( scope ), spell_value = params[0]]( dialogue const & d ) {
        const spell_id spell( spell_value.str( d ) );
        if( spell == spell_id::NULL_ID() ) {
            return d.actor( beta )->get_highest_spell_level();
        } else {
            return d.actor( beta )->get_spell_level( spell );
        }
    };
}

std::function<void( dialogue &, double )> spell_level_ass( char scope,
        std::vector<diag_value> const &params, diag_kwargs const &/* kwargs */ )
{
    return[beta = is_beta( scope ), spell_value = params[0]]( dialogue const & d, double val ) {
        const spell_id spell( spell_value.str( d ) );
        if( spell == spell_id::NULL_ID() ) {
            debugmsg( "Can't set spell level of %s", spell.str() );
        } else {
            d.actor( beta )->set_spell_level( spell, val );
        }
        return val;
    };
}

std::function<double( dialogue & )> spell_level_adjustment_eval( char scope,
        std::vector<diag_value> const &params, diag_kwargs const &/* kwargs */ )
{
    return[beta = is_beta( scope ), spell_value = params[0]]( dialogue const & d ) {
        const Character *ch = d.actor( beta )->get_character();
        if( ch ) {
            const spell_id spell( spell_value.str( d ) );
            if( spell == spell_id::NULL_ID() ) {
                return ch->magic->caster_level_adjustment;
            } else {
                std::map<spell_id, double>::iterator it = ch->magic->caster_level_adjustment_by_spell.find( spell );
                if( it != ch->magic->caster_level_adjustment_by_spell.end() ) {
                    return it->second;
                }
            }
        }
        return 0.0;
    };
}

std::function<void( dialogue &, double )> spell_level_adjustment_ass( char scope,
        std::vector<diag_value> const &params, diag_kwargs const &/* kwargs */ )
{
    return[beta = is_beta( scope ), spell_value = params[0]]( dialogue const & d, double val ) {
        const Character *ch = d.actor( beta )->get_character();
        if( ch ) {
            const spell_id spell( spell_value.str( d ) );
            if( spell == spell_id::NULL_ID() ) {
                ch->magic->caster_level_adjustment = val;
            } else {
                std::map<spell_id, double>::iterator it = ch->magic->caster_level_adjustment_by_spell.find( spell );
                if( it != ch->magic->caster_level_adjustment_by_spell.end() ) {
                    it->second = val;
                } else {
                    ch->magic->caster_level_adjustment_by_spell.insert( { spell, val } );
                }
            }
            return val;
        }
        return 0.0;
    };
}

std::function<double( dialogue & )> proficiency_eval( char scope,
        std::vector<diag_value> const &params, diag_kwargs const &kwargs )
{
    diag_value fmt_val( std::string{"time_spent"} );
    if( kwargs.count( "format" ) != 0 ) {
        fmt_val = *kwargs.at( "format" );
    }
    return [beta = is_beta( scope ), prof_value = params[0], fmt_val]( dialogue const & d ) {
        proficiency_id prof( prof_value.str( d ) );
        time_duration raw = d.actor( beta )->proficiency_practiced_time( prof );
        std::string const format = fmt_val.str( d );
        if( format == "percent" ) {
            return static_cast<int>( raw * 100  / prof->time_to_learn() );
        } else if( format == "permille" ) {
            return static_cast<int>( raw * 1000  / prof->time_to_learn() );
        } else if( format == "total_time_required" ) {
            return to_turns<int>( prof->time_to_learn() );
        } else if( format == "time_left" ) {
            return to_turns<int>( prof->time_to_learn() - raw );
        } else {
            if( format != "time_spent" ) {
                debugmsg( R"(Unknown format type "%s" for proficiency, assumning "time_spent")", format );
            }
            return to_turns<int>( raw );
        }
        return 0;
    };
}

std::function<void( dialogue &, double )> proficiency_ass( char scope,
        std::vector<diag_value> const &params, diag_kwargs const &kwargs )
{
    diag_value fmt_val( std::string{"time_spent"} );
    if( kwargs.count( "format" ) != 0 ) {
        fmt_val = *kwargs.at( "format" );
    }
    return [prof_value = params[0], fmt_val, beta = is_beta( scope )]( dialogue const & d,
    double val ) {
        proficiency_id prof( prof_value.str( d ) );
        std::string const format = fmt_val.str( d );
        int to_write = 0;
        if( format == "percent" ) {
            to_write = to_turns<int>( prof->time_to_learn() * val ) / 100;
        } else if( format == "permille" ) {
            to_write = to_turns<int>( prof->time_to_learn() * val ) / 1000;
        } else if( format == "time_left" ) {
            to_write = to_turns<int>( prof->time_to_learn() ) - val;
        } else {
            if( format != "time_spent" ) {
                debugmsg( R"(Unknown format type "%s" for proficiency, assumning "time_spent")", format );
            }
            to_write = val;
        }
        d.actor( beta )->set_proficiency_practiced_time( prof, to_write );
        return 0;
    };
}

namespace
{
double _test_add( diag_value const &v, dialogue const &d )
{
    double ret{};
    if( v.is_array() ) {
        for( diag_value const &w : v.array() ) {
            ret += _test_add( w, d );
        }
    } else {
        ret += v.dbl( d );
    }
    return ret;
}
double _test_len( diag_value const &v, dialogue const &d )
{
    double ret{};
    for( diag_value const &w : v.array( d ) ) {
        ret += w.str( d ).length();
    }
    return ret;
}
std::function<double( dialogue & )> _test_func( std::vector<diag_value> const &params,
        diag_kwargs const &kwargs,
        double ( *f )( diag_value const &v, dialogue const &d ) )
{
    std::vector<diag_value> all_params( params );
    for( diag_kwargs::value_type const &v : kwargs ) {
        if( v.first != "test_unused_kwarg" ) {
            all_params.emplace_back( *v.second );
        }
    }
    return [all_params, f]( dialogue const & d ) {
        double ret = 0;
        for( diag_value const &v : all_params ) {
            ret += f( v, d );
        }
        return ret;
    };
}
} // namespace

std::function<double( dialogue & )> test_diag( char /* scope */,
        std::vector<diag_value> const &params, diag_kwargs const &kwargs )
{
    return _test_func( params, kwargs, _test_add );
}

std::function<double( dialogue & )> test_str_len( char /* scope */,
        std::vector<diag_value> const &params, diag_kwargs const &kwargs )
{
    return _test_func( params, kwargs, _test_len );
}

std::function<double( dialogue & )> vitamin_eval( char scope,
        std::vector<diag_value> const &params, diag_kwargs const &/* kwargs */ )
{
    return[beta = is_beta( scope ), id = params[0]]( dialogue const & d ) {
        if( d.actor( beta )->get_character() ) {
            return d.actor( beta )->get_character()->vitamin_get( vitamin_id( id.str( d ) ) );
        }
        return 0;
    };
}

std::function<void( dialogue &, double )> vitamin_ass( char scope,
        std::vector<diag_value> const &params, diag_kwargs const &/* kwargs */ )
{
    return[beta = is_beta( scope ), id = params[0]]( dialogue const & d, double val ) {
        if( d.actor( beta )->get_character() ) {
            d.actor( beta )->get_character()->vitamin_set( vitamin_id( id.str( d ) ), val );
        }
    };
}

std::function<double( dialogue & )> warmth_eval( char scope,
        std::vector<diag_value> const &params, diag_kwargs const &/* kwargs */ )
{
    return[bpid = params[0], beta = is_beta( scope )]( dialogue const & d ) {
        bodypart_id bp( bpid.str( d ) );
        return units::to_legacy_bodypart_temp( d.actor( beta )->get_cur_part_temp( bp ) );
    };
}

std::function<double( dialogue & )> weather_eval( char /* scope */,
        std::vector<diag_value> const &params, diag_kwargs const &/* kwargs */ )
{
    if( params[0] == "temperature" ) {
        return []( dialogue const & ) {
            return units::to_kelvin( get_weather().weather_precise->temperature );
        };
    }
    if( params[0] == "windpower" ) {
        return []( dialogue const & ) {
            return get_weather().weather_precise->windpower;
        };
    }
    if( params[0] == "humidity" ) {
        return []( dialogue const & ) {
            return get_weather().weather_precise->humidity;
        };
    }
    if( params[0] == "pressure" ) {
        return []( dialogue const & ) {
            return get_weather().weather_precise->pressure;
        };
    }
    if( params[0] == "precipitation" ) {
        return []( dialogue const & ) {
            return precip_mm_per_hour( get_weather().weather_id->precip );
        };
    }
    throw std::invalid_argument( string_format( "Unknown weather aspect %s", params[0].str() ) );
}

std::function<void( dialogue &, double )> weather_ass( char /* scope */,
        std::vector<diag_value> const &params, diag_kwargs const &/* kwargs */ )
{
    if( params[0] == "temperature" ) {
        return []( dialogue const &, double val ) {
            get_weather().weather_precise->temperature = units::from_kelvin( val );
            get_weather().temperature = units::from_kelvin( val );
            get_weather().clear_temp_cache();
        };
    }
    if( params[0] == "windpower" ) {
        return []( dialogue const &, double val ) {
            get_weather().weather_precise->windpower = val;
            get_weather().clear_temp_cache();
        };
    }
    if( params[0] == "humidity" ) {
        return []( dialogue const &, double val ) {
            get_weather().weather_precise->humidity = val;
            get_weather().clear_temp_cache();
        };
    }
    if( params[0] == "pressure" ) {
        return []( dialogue const &, double val ) {
            get_weather().weather_precise->pressure = val;
            get_weather().clear_temp_cache();
        };
    }
    throw std::invalid_argument( string_format( "Unknown weather aspect %s", params[0].str() ) );
}
