#include "dialogue_chatbin.h"
#include "mission.h"

void dialogue_chatbin::add_new_mission( mission *miss )
{
    if( miss == nullptr ) {
        return;
    }
    missions.push_back( miss );
}

void dialogue_chatbin::check_missions()
{
    // TODO: or simply fail them? Some missions might only need to be reported.
    auto &ma = missions_assigned;
    const auto last = std::remove_if( ma.begin(), ma.end(), []( class mission const * m ) {
        return !m->is_assigned();
    } );
    std::copy( last, ma.end(), std::back_inserter( missions ) );
    ma.erase( last, ma.end() );
}

void dialogue_chatbin::store_chosen_training( const skill_id &c_skill, const matype_id &c_style,
        const spell_id &c_spell )
{
    clear_training();
    if( c_skill ) {
        skill = c_skill;
    } else if( c_style ) {
        style = c_style;
    } else if( c_spell != spell_id() ) {
        dialogue_spell = c_spell;
    }
}

void dialogue_chatbin::clear_training()
{
    style = matype_id::NULL_ID();
    skill = skill_id::NULL_ID();
    dialogue_spell = spell_id();
}

void dialogue_chatbin::clear_all()
{
    clear_training();
    missions.clear();
    missions_assigned.clear();
    mission_selected = nullptr;
}
