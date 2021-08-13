#include "character.h"
#include "creature.h"
#include "item.h"
#include "talker_eoc.h"
#include "type_id.h"

player *talker_eoc::get_character()
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
    return nullptr;
}
player *talker_eoc::get_character() const
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
    return nullptr;
}
npc *talker_eoc::get_npc()
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
    return nullptr;
}
npc *talker_eoc::get_npc() const
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
    return nullptr;
}
item_location *talker_eoc::get_item()
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
    return nullptr;
}
item_location *talker_eoc::get_item() const
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
    return nullptr;
}
monster *talker_eoc::get_monster()
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
    return nullptr;
}
monster *talker_eoc::get_monster() const
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
    return nullptr;
}
Creature *talker_eoc::get_creature()
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
    return nullptr;
}
Creature *talker_eoc::get_creature() const
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
    return nullptr;
}
// identity and location
std::string talker_eoc::disp_name() const
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
    return "";
}
character_id talker_eoc::getID() const
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
    return character_id( 0 );
}
bool talker_eoc::is_male() const
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
    return false;
}
std::vector<std::string> talker_eoc::get_grammatical_genders() const
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
    return {};
}
std::string talker_eoc::distance_to_goal() const
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
    return "";
}

// mandatory functions for starting a dialogue
bool talker_eoc::will_talk_to_u( const player &, bool )
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
    return false;
}
std::vector<std::string> talker_eoc::get_topics( bool )
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
    return {};
}
void talker_eoc::check_missions()
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
}
void talker_eoc::update_missions( const std::vector<mission *> & )
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
}
bool talker_eoc::check_hostile_response( int ) const
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
    return false;
}
int talker_eoc::parse_mod( const std::string &, int ) const
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
    return 0;
}
int talker_eoc::trial_chance_mod( const std::string & ) const
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
    return 0;
}

// stats, skills, traits, bionics, and magic
int talker_eoc::str_cur() const
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
    return 0;
}
int talker_eoc::dex_cur() const
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
    return 0;
}
int talker_eoc::int_cur() const
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
    return 0;
}
int talker_eoc::per_cur() const
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
    return 0;
}
int talker_eoc::get_skill_level( const skill_id & ) const
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
    return 0;
}
bool talker_eoc::has_trait( const trait_id & ) const
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
    return false;
}
void talker_eoc::set_mutation( const trait_id & )
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
}
void talker_eoc::unset_mutation( const trait_id & )
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
}
void talker_eoc::mod_fatigue( int )
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
}
bool talker_eoc::has_trait_flag( const json_character_flag & ) const
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
    return false;
}
bool talker_eoc::crossed_threshold() const
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
    return false;
}
int talker_eoc::num_bionics() const
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
    return 0;
}
bool talker_eoc::has_max_power() const
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
    return false;
}
bool talker_eoc::has_bionic( const bionic_id & ) const
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
    return false;
}
bool talker_eoc::knows_spell( const spell_id & ) const
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
    return false;
}
bool talker_eoc::knows_proficiency( const proficiency_id & ) const
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
    return false;
}
std::vector<skill_id> talker_eoc::skills_offered_to( const talker & ) const
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
    return {};
}
std::string talker_eoc::skill_training_text( const talker &, const skill_id & ) const
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
    return {};
}
std::vector<proficiency_id> talker_eoc::proficiencies_offered_to( const talker & ) const
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
    return {};
}
std::string talker_eoc::proficiency_training_text( const talker &,
        const proficiency_id & ) const
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
    return {};
}
std::vector<matype_id> talker_eoc::styles_offered_to( const talker & ) const
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
    return {};
}
std::string talker_eoc::style_training_text( const talker &, const matype_id & ) const
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
    return {};
}
std::vector<spell_id> talker_eoc::spells_offered_to( talker & )
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
    return {};
}
std::string talker_eoc::spell_training_text( talker &, const spell_id & )
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
    return {};
}
void talker_eoc::store_chosen_training( const skill_id &, const matype_id &,
                                        const spell_id &, const proficiency_id & )
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
}

// effects and values
bool talker_eoc::has_effect( const efftype_id & ) const
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
    return false;
}
bool talker_eoc::is_deaf() const
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
    return false;
}
bool talker_eoc::can_see() const
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
    return false;
}
bool talker_eoc::is_mute() const
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
    return false;
}
void talker_eoc::add_effect( const efftype_id &, const time_duration &, std::string, bool, bool,
                             int )
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
}
void talker_eoc::remove_effect( const efftype_id & )
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
}
std::string talker_eoc::get_value( const std::string & ) const
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
    return "";
}
void talker_eoc::set_value( const std::string &, const std::string & )
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
}
void talker_eoc::remove_value( const std::string & )
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
}

// inventory, buying, and selling
bool talker_eoc::is_wearing( const itype_id & ) const
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
    return false;
}
int talker_eoc::charges_of( const itype_id & ) const
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
    return 0;
}
bool talker_eoc::has_charges( const itype_id &, int ) const
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
    return false;
}
std::list<item> talker_eoc::use_charges( const itype_id &, int )
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
    return {};
}
bool talker_eoc::has_amount( const itype_id &, int ) const
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
    return false;
}
std::list<item> talker_eoc::use_amount( const itype_id &, int )
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
    return {};
}
int talker_eoc::value( const item & ) const
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
    return 0;
}
int talker_eoc::cash() const
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
    return 0;
}
int talker_eoc::debt() const
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
    return 0;
}
void talker_eoc::add_debt( int )
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
}
std::vector<item *> talker_eoc::items_with( const std::function<bool( const item & )> & ) const

{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
    return {};
}
void talker_eoc::i_add( const item & )
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
}
void talker_eoc::remove_items_with( const std::function<bool( const item & )> & )
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
}
bool talker_eoc::unarmed_attack() const
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
    return false;
}
bool talker_eoc::can_stash_weapon() const
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
    return false;
}
bool talker_eoc::has_stolen_item( const talker & ) const
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
    return false;
}
int talker_eoc::cash_to_favor( int ) const
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
    return 0;
}
std::string talker_eoc::give_item_to( bool )
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
    return _( "Nope." );
}
bool talker_eoc::buy_from( int )
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
    return false;
}
void talker_eoc::buy_monster( talker &, const mtype_id &, int, int, bool,
                              const translation & )
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
}

// missions
std::vector<mission *> talker_eoc::available_missions() const
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
    return {};
}
std::vector<mission *> talker_eoc::assigned_missions() const
{
    // Don't complain here - this is always called in dialogue
    // TODO: Fix that and complain here
    //debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
    return {};
}
mission *talker_eoc::selected_mission() const
{
    // Don't complain here - this is always called in dialogue
    // TODO: Fix that and complain here
    //debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
    return nullptr;
}
void talker_eoc::select_mission( mission * )
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
}
void talker_eoc::add_mission( const mission_type_id & )
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
}
void talker_eoc::set_companion_mission( const std::string & )
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
}

// factions and alliances
faction *talker_eoc::get_faction() const
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
    return nullptr;
}
void talker_eoc::set_fac( const faction_id & )
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
}
void talker_eoc::add_faction_rep( int )
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
}
bool talker_eoc::is_following() const
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
    return false;
}
bool talker_eoc::is_friendly( const Character & )  const
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
    return false;
}
bool talker_eoc::is_player_ally()  const
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
    return false;
}
bool talker_eoc::turned_hostile() const
{
    // Don't complain here - this is always called in dialogue
    // TODO: Fix that and complain here
    //debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
    return false;
}
bool talker_eoc::is_enemy() const
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
    return false;
}
void talker_eoc::make_angry()
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
}

// ai rules
bool talker_eoc::has_ai_rule( const std::string &, const std::string & ) const
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
    return false;
}
void talker_eoc::toggle_ai_rule( const std::string &, const std::string & )
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
}
void talker_eoc::set_ai_rule( const std::string &, const std::string & )
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
}
void talker_eoc::clear_ai_rule( const std::string &, const std::string & )
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
}

// other descriptors
std::string talker_eoc::get_job_description() const
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
    return "";
}
std::string talker_eoc::evaluation_by( const talker & ) const
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
    return "";
}
std::string talker_eoc::short_description() const
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
    return "";
}
bool talker_eoc::has_activity() const
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
    return false;
}
bool talker_eoc::is_mounted() const
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
    return false;
}
bool talker_eoc::is_myclass( const npc_class_id & ) const
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
    return false;
}
void talker_eoc::set_class( const npc_class_id & )
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
}
int talker_eoc::get_fatigue() const
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
    return 0;
}
int talker_eoc::get_hunger() const
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
    return 0;
}
int talker_eoc::get_thirst() const
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
    return 0;
}
bool talker_eoc::is_in_control_of( const vehicle & ) const
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
    return false;
}

// speaking
void talker_eoc::say( const std::string & )
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
}
void talker_eoc::shout( const std::string &, bool )
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
}

// miscellaneous
bool talker_eoc::enslave_mind()
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
    return false;
}
std::string talker_eoc::opinion_text() const
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
    return "";
}
void talker_eoc::add_opinion( int /*trust*/, int /*fear*/, int /*value*/, int /*anger*/,
                              int /*debt*/ )
{
    // Don't complain here - this is always called in dialogue
    // TODO: Fix that and complain here
    //debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
}
void talker_eoc::set_first_topic( const std::string & )
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
}
bool talker_eoc::is_safe() const
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
    return true;
}
void talker_eoc::mod_pain( int )
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
}
int talker_eoc::pain_cur() const
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
    return 0;
}
bool talker_eoc::worn_with_flag( const flag_id & ) const
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
    return false;
}
bool talker_eoc::wielded_with_flag( const flag_id & ) const
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
    return false;
}
units::energy talker_eoc::power_cur() const
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
    return 0_kJ;
}
void talker_eoc::mod_healthy_mod( int, int )
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
}
int talker_eoc::morale_cur() const
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
    return 0;
}
int talker_eoc::focus_cur() const
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
    return 0;
}
void talker_eoc::mod_focus( int )
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
}
void talker_eoc::add_morale( const morale_type &, int, int, time_duration, time_duration,
                             bool )
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
}
void talker_eoc::remove_morale( const morale_type & )
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
}
int talker_eoc::posx() const
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
    return 0;
}
int talker_eoc::posy() const
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
    return 0;
}
int talker_eoc::posz() const
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
    return 0;
}
tripoint talker_eoc::pos() const
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
    return tripoint_zero;
}
tripoint_abs_omt talker_eoc::global_omt_location() const
{
    debugmsg( "talker_eoc '%s' called '%s'", *_eoc_name, __FUNCTION_NAME__ );
    return get_player_character().global_omt_location();
}
