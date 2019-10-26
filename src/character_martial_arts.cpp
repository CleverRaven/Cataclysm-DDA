#include "character_martial_arts.h"

#include "action.h"
#include "martialarts.h"
#include "messages.h"
#include "output.h"

const matype_id style_none( "style_none" );
const matype_id style_kicks( "style_kicks" );

using itype_id = std::string;

character_martial_arts::character_martial_arts()
{

    keep_hands_free = false;

    style_selected = style_none;

    ma_styles = { {
            style_none, style_kicks
        }
    };
}

bool character_martial_arts::selected_allow_melee() const
{
    return style_selected->allow_melee;
}

bool character_martial_arts::selected_strictly_melee() const
{
    return style_selected->strictly_melee;
}

bool character_martial_arts::selected_has_weapon( const itype_id &weap ) const
{
    return style_selected->has_weapon( weap );
}

bool character_martial_arts::selected_force_unarmed() const
{
    return style_selected->force_unarmed;
}

bool character_martial_arts::knows_selected_style() const
{
    return has_martialart( style_selected );
}

bool character_martial_arts::selected_is_none() const
{
    return style_selected == style_none;
}

void character_martial_arts::learn_current_style_CQB( bool is_avatar )
{
    add_martialart( style_selected );
    if( is_avatar ) {
        add_msg( m_good, _( "You have learned %s from extensive practice with the CQB Bionic." ),
                 style_selected->name );
    }
}

void character_martial_arts::learn_style( const matype_id &mastyle, bool is_avatar )
{
    add_martialart( mastyle );

    if( is_avatar ) {
        add_msg( m_good, _( "You learn %s." ),
                 mastyle->name );
        add_msg( m_info, _( "%s to select martial arts style." ),
                 press_x( ACTION_PICK_STYLE ) );
    }
}

void character_martial_arts::set_style( const matype_id &mastyle, bool force )
{
    if( force || has_martialart( mastyle ) ) {
        style_selected = mastyle;
    }
}

void character_martial_arts::reset_style()
{
    style_selected = style_none;
}

void character_martial_arts::selected_style_check()
{
    // check if player knows current style naturally, otherwise drop them back to style_none
    if( style_selected != style_none && style_selected != style_kicks ) {
        bool has_style = false;
        for( const matype_id &elem : ma_styles ) {
            if( elem == style_selected ) {
                has_style = true;
            }
        }
        if( !has_style ) {
            reset_style();
        }
    }
}

std::string character_martial_arts::enumerate_known_styles( const itype_id &itt ) const
{
    return enumerate_as_string( ma_styles.begin(), ma_styles.end(),
    [itt]( const matype_id & mid ) {
        return mid->has_weapon( itt ) ? mid->name.translated() : std::string();
    } );
}

std::string character_martial_arts::selected_style_name( const Character &owner ) const
{
    if( style_selected->force_unarmed || style_selected->weapon_valid( owner.weapon ) ) {
        return style_selected->name.translated();
    } else if( owner.is_armed() ) {
        return _( "Normal" );
    } else {
        return _( "No Style" );
    }
}

std::vector<matype_id> character_martial_arts::get_unknown_styles( const character_martial_arts
        &from ) const
{
    std::vector<matype_id> ret;
    for( const matype_id &i : from.ma_styles ) {
        if( !has_martialart( i ) ) {
            ret.push_back( i );
        }
    }
    return ret;
}
