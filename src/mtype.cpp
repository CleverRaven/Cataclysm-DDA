#include "mtype.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <unordered_map>

#include "behavior_strategy.h"
#include "creature.h"
#include "field_type.h"
#include "item.h"
#include "itype.h"
#include "mod_manager.h"
#include "mondeath.h"
#include "monstergenerator.h"
#include "output.h"
#include "translations.h"
#include "units.h"
#include "weakpoint.h"

static const harvest_id harvest_list_human( "human" );

static const itype_id itype_bone( "bone" );
static const itype_id itype_bone_tainted( "bone_tainted" );
static const itype_id itype_fish( "fish" );
static const itype_id itype_human_flesh( "human_flesh" );
static const itype_id itype_meat( "meat" );
static const itype_id itype_meat_tainted( "meat_tainted" );
static const itype_id itype_veggy( "veggy" );
static const itype_id itype_veggy_tainted( "veggy_tainted" );

static const material_id material_bone( "bone" );
static const material_id material_flesh( "flesh" );
static const material_id material_hflesh( "hflesh" );
static const material_id material_iflesh( "iflesh" );
static const material_id material_veggy( "veggy" );

static const species_id species_MOLLUSK( "MOLLUSK" );

mtype::mtype()
{
    id = mtype_id::NULL_ID();
    name = pl_translation( "human", "humans" );
    sym = " ";
    color = c_white;
    size = creature_size::medium;
    volume = 62499_ml;
    weight = 81499_gram;
    mat = { { material_flesh, 1 } };
    phase = phase_id::SOLID;
    def_chance = 0;
    upgrades = false;
    upgrade_multi_range = cata::optional<int>();
    half_life = -1;
    age_grow = -1;
    upgrade_into = mtype_id::NULL_ID();
    upgrade_group = mongroup_id::NULL_ID();

    reproduces = false;
    baby_count = -1;
    baby_monster = mtype_id::NULL_ID();
    baby_egg = itype_id::NULL_ID();

    biosignatures = false;
    biosig_item = itype_id::NULL_ID();

    burn_into = mtype_id::NULL_ID();
    sp_defense = nullptr;
    melee_training_cap = MAX_SKILL;
    harvest = harvest_list_human;
    luminance = 0;
    bash_skill = 0;

    aggro_character = true;

    flags
    .set( MF_HUMAN );
}

std::string mtype::nname( unsigned int quantity ) const
{
    return name.translated( quantity );
}

bool mtype::has_special_attack( const std::string &attack_name ) const
{
    return special_attacks.find( attack_name ) != special_attacks.end();
}

bool mtype::has_flag( m_flag flag ) const
{
    MonsterGenerator::generator().m_flag_usage_stats[flag]++;
    return flags[flag];
}

void mtype::set_flag( m_flag flag, bool state )
{
    flags.set( flag, state );
}

bool mtype::made_of( const material_id &material ) const
{
    return mat.find( material ) != mat.end();
}

bool mtype::made_of_any( const std::set<material_id> &materials ) const
{
    if( mat.empty() ) {
        return false;
    }

    return std::any_of( mat.begin(), mat.end(), [&materials]( const std::pair<material_id, int> &e ) {
        return materials.count( e.first );
    } );
}

bool mtype::has_anger_trigger( mon_trigger trigger ) const
{
    return anger[trigger];
}

bool mtype::has_fear_trigger( mon_trigger trigger ) const
{
    return fear[trigger];
}

bool mtype::has_placate_trigger( mon_trigger trigger ) const
{
    return placate[trigger];
}

bool mtype::in_category( const std::string &category ) const
{
    return categories.find( category ) != categories.end();
}

bool mtype::in_species( const species_id &spec ) const
{
    return species.count( spec ) > 0;
}

std::vector<std::string> mtype::species_descriptions() const
{
    std::vector<std::string> ret;
    for( const species_id &s : species ) {
        if( !s->description.empty() ) {
            ret.emplace_back( s->description.translated() );
        }
    }
    return ret;
}

field_type_id mtype::get_bleed_type() const
{
    for( const species_id &s : species ) {
        if( !s->bleeds.is_empty() ) {
            return s->bleeds;
        }
    }
    return fd_null;
}

bool mtype::same_species( const mtype &other ) const
{
    return std::any_of( species.begin(), species.end(), [&]( const species_id & s ) {
        return other.in_species( s );
    } );
}

field_type_id mtype::bloodType() const
{
    if( has_flag( MF_ACID_BLOOD ) )
        //A monster that has the death effect "ACID" does not need to have acid blood.
    {
        return fd_acid;
    }
    if( has_flag( MF_BILE_BLOOD ) ) {
        return fd_bile;
    }
    if( has_flag( MF_LARVA ) || has_flag( MF_ARTHROPOD_BLOOD ) ) {
        return fd_blood_invertebrate;
    }
    if( made_of( material_veggy ) ) {
        return fd_blood_veggy;
    }
    if( made_of( material_iflesh ) ) {
        return fd_blood_insect;
    }
    if( has_flag( MF_WARM ) && made_of( material_flesh ) ) {
        return fd_blood;
    }
    return get_bleed_type();
}

field_type_id mtype::gibType() const
{
    if( has_flag( MF_LARVA ) || in_species( species_MOLLUSK ) ) {
        return fd_gibs_invertebrate;
    }
    if( made_of( material_veggy ) ) {
        return fd_gibs_veggy;
    }
    if( made_of( material_iflesh ) ) {
        return fd_gibs_insect;
    }
    if( made_of( material_flesh ) ) {
        return fd_gibs_flesh;
    }
    // There are other materials not listed here like steel, protoplasmic, powder, null, stone, bone
    return fd_null;
}

itype_id mtype::get_meat_itype() const
{
    if( has_flag( MF_POISON ) ) {
        if( made_of( material_flesh ) || made_of( material_hflesh ) ||
            //In the future, insects could drop insect flesh rather than plain ol' meat.
            made_of( material_iflesh ) ) {
            return itype_meat_tainted;
        } else if( made_of( material_veggy ) ) {
            return itype_veggy_tainted;
        } else if( made_of( material_bone ) ) {
            return itype_bone_tainted;
        }
    } else {
        if( made_of( material_flesh ) || made_of( material_hflesh ) ) {
            if( has_flag( MF_HUMAN ) ) {
                return itype_human_flesh;
            } else if( has_flag( MF_AQUATIC ) ) {
                return itype_fish;
            } else {
                return itype_meat;
            }
        } else if( made_of( material_iflesh ) ) {
            //In the future, insects could drop insect flesh rather than plain ol' meat.
            return itype_meat;
        } else if( made_of( material_veggy ) ) {
            return itype_veggy;
        } else if( made_of( material_bone ) ) {
            return itype_bone;
        }
    }
    return itype_id::NULL_ID();
}

int mtype::get_meat_chunks_count() const
{
    const float ch = to_gram( weight ) * ( 0.40f - 0.02f * std::log10( to_gram( weight ) ) );
    const itype *chunk = item::find_type( get_meat_itype() );
    return static_cast<int>( ch / to_gram( chunk->weight ) );
}

std::string mtype::get_description() const
{
    return description.translated();
}

ascii_art_id mtype::get_picture_id() const
{
    return picture_id;
}

std::string mtype::get_footsteps() const
{
    if( !species.empty() ) {
        return species.begin()->obj().get_footsteps();
    }
    return _( "footsteps." );
}

void mtype::set_strategy()
{
    goals.set_strategy( behavior::strategy_map[ "sequential_until_done" ] );
}

void mtype::add_goal( const std::string &goal_id )
{
    goals.add_child( &string_id<behavior::node_t>( goal_id ).obj() );
}

const behavior::node_t *mtype::get_goals() const
{
    return &goals;
}

void mtype::faction_display( catacurses::window &w, const point &top_left, const int width ) const
{
    int y = 0;
    // Name & symbol
    trim_and_print( w, top_left + point( 2, y ), width, c_white, string_format( "%s  %s", colorize( sym,
                    color ), nname() ) );
    y++;
    // Difficulty
    std::string diff_str;
    if( difficulty < 3 ) {
        diff_str = _( "<color_light_gray>Minimal threat.</color>" );
    } else if( difficulty < 10 ) {
        diff_str = _( "<color_light_gray>Mildly dangerous.</color>" );
    } else if( difficulty < 20 ) {
        diff_str = _( "<color_light_red>Dangerous.</color>" );
    } else if( difficulty < 30 ) {
        diff_str = _( "<color_red>Very dangerous.</color>" );
    } else if( difficulty < 50 ) {
        diff_str = _( "<color_red>Extremely dangerous.</color>" );
    } else {
        diff_str = _( "<color_red>Fatally dangerous!</color>" );
    }
    trim_and_print( w, top_left + point( 0, ++y ), width, c_light_gray,
                    string_format( "%s: %s", colorize( _( "Difficulty" ), c_white ), diff_str ) );
    // Origin
    std::vector<std::string> origin_list =
        foldstring( string_format( "%s: %s", colorize( _( "Origin" ), c_white ),
                                   enumerate_as_string( src.begin(), src.end(),
    []( const std::pair<mtype_id, mod_id> &source ) {
        return string_format( "'%s'", source.second->name() );
    }, enumeration_conjunction::arrow ) ), width );
    for( const std::string &org : origin_list ) {
        trim_and_print( w, top_left + point( 0, ++y ), width, c_light_gray, org );
    }
    // Size
    const std::map<creature_size, std::string> size_map = {
        {creature_size::tiny, translate_marker( "Tiny" )},
        {creature_size::small, translate_marker( "Small" )},
        {creature_size::medium, translate_marker( "Medium" )},
        {creature_size::large, translate_marker( "Large" )},
        {creature_size::huge, translate_marker( "Huge" )}
    };
    auto size_iter = size_map.find( size );
    trim_and_print( w, top_left + point( 0, ++y ), width, c_light_gray,
                    string_format( "%s: %s", colorize( _( "Size" ), c_white ),
                                   size_iter == size_map.end() ? _( "Unknown" ) : _( size_iter->second ) ) );
    // Species
    std::vector<std::string> species_list =
        foldstring( string_format( "%s: %s", colorize( _( "Species" ), c_white ),
    enumerate_as_string( species_descriptions(), []( const std::string & sp ) {
        return colorize( sp, c_yellow );
    } ) ), width );
    for( const std::string &sp : species_list ) {
        trim_and_print( w, top_left + point( 0, ++y ), width, c_light_gray, sp );
    }
    // Senses
    std::vector<std::string> senses_str;
    if( has_flag( MF_HEARS ) ) {
        senses_str.emplace_back( colorize( _( "hearing" ), c_yellow ) );
    }
    if( has_flag( MF_SEES ) ) {
        senses_str.emplace_back( colorize( _( "sight" ), c_yellow ) );
    }
    if( has_flag( MF_SMELLS ) ) {
        senses_str.emplace_back( colorize( _( "smell" ), c_yellow ) );
    }
    trim_and_print( w, top_left + point( 0, ++y ), width, c_light_gray,
                    string_format( "%s: %s", colorize( _( "Senses" ), c_white ), enumerate_as_string( senses_str ) ) );
    // Abilities
    if( has_flag( MF_SWIMS ) ) {
        trim_and_print( w, top_left + point( 0, ++y ), width, c_white, _( "It can swim." ) );
    }
    if( has_flag( MF_FLIES ) ) {
        trim_and_print( w, top_left + point( 0, ++y ), width, c_white, _( "It can fly." ) );
    }
    if( has_flag( MF_DIGS ) ) {
        trim_and_print( w, top_left + point( 0, ++y ), width, c_white, _( "It can burrow underground." ) );
    }
    if( has_flag( MF_CLIMBS ) ) {
        trim_and_print( w, top_left + point( 0, ++y ), width, c_white, _( "It can climb." ) );
    }
    if( has_flag( MF_GRABS ) ) {
        trim_and_print( w, top_left + point( 0, ++y ), width, c_white, _( "It can grab." ) );
    }
    if( has_flag( MF_VENOM ) ) {
        trim_and_print( w, top_left + point( 0, ++y ), width, c_white, _( "It can inflict poison." ) );
    }
    if( has_flag( MF_PARALYZE ) ) {
        trim_and_print( w, top_left + point( 0, ++y ), width, c_white, _( "It can inflict paralysis." ) );
    }
    // Description
    fold_and_print( w, top_left + point( 0, y + 2 ), width, c_light_gray, get_description() );
}
