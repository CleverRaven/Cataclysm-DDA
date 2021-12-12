#include "character.h"
#include "character_modifier.h"
#include "generic_factory.h"
#include "move_mode.h"

static const character_modifier_id
character_modifier_limb_footing_movecost_mod( "limb_footing_movecost_mod" );
static const character_modifier_id
character_modifier_limb_speed_movecost_mod( "limb_speed_movecost_mod" );

static const limb_score_id limb_score_manip( "manip" );
static const limb_score_id limb_score_swim( "swim" );

static const skill_id skill_archery( "archery" );
static const skill_id skill_swimming( "swimming" );

namespace
{

generic_factory<character_modifier> character_modifier_factory( "character modifier" );

} // namespace

/** @relates string_id */
template<>
const character_modifier &string_id<character_modifier>::obj() const
{
    return character_modifier_factory.obj( *this );
}

/** @relates string_id */
template<>
bool string_id<character_modifier>::is_valid() const
{
    return character_modifier_factory.is_valid( *this );
}

void character_modifier::load_character_modifiers( const JsonObject &jo, const std::string &src )
{
    character_modifier_factory.load( jo, src );
}

void character_modifier::reset()
{
    character_modifier_factory.reset();
}

const std::vector<character_modifier> &character_modifier::get_all()
{
    return character_modifier_factory.get_all();
}

static character_modifier::mod_type string_to_modtype( const std::string &s )
{
    static const std::map<std::string, character_modifier::mod_type> modtype_map = {
        { "+", character_modifier::ADD },
        { "x", character_modifier::MULT },
        { "*", character_modifier::MULT },
        { "", character_modifier::NONE }
    };
    const auto &iter = modtype_map.find( s );
    if( iter == modtype_map.end() ) {
        debugmsg( "Invalid mod_type %s", s );
        return character_modifier::NONE;
    }
    return iter->second;
}

static float load_float_or_maxmovecost( const JsonObject &jo, const std::string &field )
{
    float val = 0.0f;
    if( jo.has_string( field ) ) {
        std::string val_str = jo.get_string( field );
        if( val_str == "max_move_cost" ) {
            val = MAX_MOVECOST_MODIFIER;
        } else {
            jo.throw_error( string_format( "invalid %s %s: use a float value or \"max_move_cost\"", field,
                                           val_str ) );
        }
    } else if( jo.has_float( field ) ) {
        mandatory( jo, false, field, val );
    }
    return val;
}

void character_modifier::load( const JsonObject &jo, const std::string & )
{
    mandatory( jo, was_loaded, "id", id );
    mandatory( jo, was_loaded, "description", desc );

    std::string modtypestr;
    optional( jo, was_loaded, "mod_type", modtypestr, "" );
    modtype = string_to_modtype( modtypestr );

    const JsonObject &jobj = jo.get_object( "value" );
    optional( jobj, was_loaded, "builtin", builtin, "" );
    if( builtin.empty() ) {
        mandatory( jobj, was_loaded, "limb_score", limbscore );
        optional( jobj, was_loaded, "limb_type", limbtype, body_part_type::type::num_types );
        if( jobj.has_member( "override_encumb" ) ) {
            bool over;
            mandatory( jobj, was_loaded, "override_encumb", over );
            override_encumb = over ? 1 : 0;
        }
        if( jobj.has_member( "override_wounds" ) ) {
            bool over;
            mandatory( jobj, was_loaded, "override_wounds", over );
            override_wounds = over ? 1 : 0;
        }
        min_val = load_float_or_maxmovecost( jobj, "min" );
        max_val = load_float_or_maxmovecost( jobj, "max" );
        optional( jobj, was_loaded, "nominator", nominator, 0.0f );
        optional( jobj, was_loaded, "subtract", subtractor, 0.0f );
    }
}

// Scores

// the total of the manipulator score in the best limb group
static float manipulator_score( const std::map<bodypart_str_id, bodypart> body )
{
    std::map<body_part_type::type, std::vector<bodypart>> bodypart_groups;
    std::vector<float> score_groups;
    for( const std::pair<const bodypart_str_id, bodypart> &id : body ) {
        bodypart_groups[id.first->limb_type].emplace_back( id.second );
    }
    for( std::pair<const body_part_type::type, std::vector<bodypart>> &part : bodypart_groups ) {
        float total = 0.0f;
        std::sort( part.second.begin(), part.second.end(),
        []( const bodypart & a, const bodypart & b ) {
            return a.get_limb_score_max( limb_score_manip ) < b.get_limb_score_max( limb_score_manip );
        } );
        for( const bodypart &id : part.second ) {
            total = std::min( total + id.get_limb_score( limb_score_manip ),
                              id.get_limb_score_max( limb_score_manip ) );
        }
        score_groups.emplace_back( total );
    }
    const auto score_groups_max = std::max_element( score_groups.begin(), score_groups.end() );
    if( score_groups_max == score_groups.end() ) {
        return 0.0f;
    } else {
        return std::max( 0.0f, *score_groups_max );
    }
}

float Character::get_limb_score( const limb_score_id &score, const body_part_type::type &bp,
                                 int override_encumb, int override_wounds ) const
{
    int skill = -1;
    // manipulator/swim scores are treated a little special for now
    if( score == limb_score_manip ) {
        return manipulator_score( body );
    } else if( score == limb_score_swim ) {
        skill = get_skill_level( skill_swimming );
    }
    float total = 0.0f;
    for( const std::pair<const bodypart_str_id, bodypart> &id : body ) {
        if( bp == body_part_type::type::num_types || id.first->limb_type == bp ) {
            total += id.second.get_limb_score( score, skill, override_encumb, override_wounds );
        }
    }
    return std::max( 0.0f, total );
}

// Modifiers

static double aim_speed_skill_modifier( const Character &c, const skill_id &gun_skill )
{
    double skill_mult = 0.25;
    double base_modifier = 0;
    if( gun_skill == skill_archery ) {
        skill_mult = 0.5;
        base_modifier = -1.5;
    }
    return skill_mult * std::min( MAX_SKILL, c.get_skill_level( gun_skill ) ) + base_modifier;
}

static double aim_speed_dex_modifier( const Character &c, const skill_id & )
{
    return ( c.get_dex() - 8 ) * 0.5;
}

static float stamina_move_cost_modifier( const Character &c, const skill_id & )
{
    // Both walk and run speed drop to half their maximums as stamina approaches 0.
    // Convert stamina to a float first to allow for decimal place carrying
    float stamina_modifier = ( static_cast<float>( c.get_stamina() ) / c.get_stamina_max() + 1 ) / 2;
    return stamina_modifier * c.move_mode->move_speed_mult();
}

static float limb_run_cost_modifier( const Character &c, const skill_id & )
{
    return ( character_modifier_limb_footing_movecost_mod->modifier( c ) +
             character_modifier_limb_speed_movecost_mod->modifier( c ) * 2 ) / 3.0f;
}

static float call_builtin( const std::string &builtin, const Character &c, const skill_id &skill )
{
    static const std::map<std::string, std::function<float ( const Character &, const skill_id & )>>
    func_map = {
        { "limb_run_cost_modifier", limb_run_cost_modifier },
        { "stamina_move_cost_modifier", stamina_move_cost_modifier },
        { "aim_speed_dex_modifier", aim_speed_dex_modifier },
        { "aim_speed_skill_modifier", aim_speed_skill_modifier }
    };

    auto iter = func_map.find( builtin );
    if( iter == func_map.end() ) {
        debugmsg( "Invalid builtin function %s for character modifier", builtin );
        return 0.0f;
    }

    return ( iter->second )( c, skill );
}

// Generally for cost mods: mod = min( max_val, ( nominator / limb_score ) - subtractor )
// for melee attack roll:   mod = max( min_val, limb_score )
// for straight limb_score: nominator == 0, subtractor == 0, max_val == 0, min_val == 0
float character_modifier::modifier( const Character &c, const skill_id &skill ) const
{
    // use builtin to calculate modifier
    if( !builtin.empty() ) {
        return call_builtin( builtin, c, skill );
    }

    float score = c.get_limb_score( limbscore, limbtype, override_encumb, override_wounds );
    // score == 0
    if( score < std::numeric_limits<float>::epsilon() ) {
        return min_val > std::numeric_limits<float>::epsilon() ? min_val :
               max_val > std::numeric_limits<float>::epsilon() ? max_val : 0.0f;
    }
    if( nominator > std::numeric_limits<float>::epsilon() ) {
        score = nominator / score;
    }
    if( subtractor > std::numeric_limits<float>::epsilon() ) {
        score -= subtractor;
    }
    if( max_val > std::numeric_limits<float>::epsilon() ) {
        score = std::min( max_val, score );
    }
    if( min_val > std::numeric_limits<float>::epsilon() ) {
        score = std::max( min_val, score );
    }
    return score;
}

float Character::get_modifier( const character_modifier_id &mod, const skill_id &skill ) const
{
    return mod->modifier( *this, skill );
}
