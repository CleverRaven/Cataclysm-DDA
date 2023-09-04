#include "skill_ui.h"

#include "type_id.h"
#include "ui.h"

std::vector<HeaderSkill> get_HeaderSkills( const std::vector<const Skill *> &sorted_skills )
{
    std::vector<HeaderSkill> skill_list;
    skill_displayType_id prev_type = skill_displayType_id::NULL_ID();
    for( const Skill * const &s : sorted_skills ) {
        if( s->display_category() != prev_type ) {
            prev_type = s->display_category();
            skill_list.emplace_back( s, true );
        }
        skill_list.emplace_back( s, false );
    }
    return skill_list;
}

template void skip_skill_headers<int>( const std::vector<HeaderSkill> &, int &, bool );
template void skip_skill_headers<unsigned>( const std::vector<HeaderSkill> &, unsigned &, bool );

template<typename V>
void skip_skill_headers( const std::vector<HeaderSkill> &skillslist, V &line, bool inc )
{
    const V prev_line = line;
    while( skillslist[line].is_header ) {
        line = inc_clamp_wrap( line, inc, skillslist.size() );
        if( line == prev_line ) {
            break;
        }
    }
}
