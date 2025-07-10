#include "diary.h"

#include <algorithm>
#include <fstream>
#include <string>
#include <utility>

#include "avatar.h"
#include "bionics.h"
#include "calendar.h"
#include "cata_path.h"
#include "cata_utility.h"
#include "catacharset.h"
#include "color.h"
#include "enum_conversions.h"
#include "filesystem.h"
#include "flexbuffer_json.h"
#include "game.h"
#include "json.h"
#include "kill_tracker.h"
#include "magic.h"
#include "mission.h"
#include "mtype.h"
#include "mutation.h"
#include "output.h"
#include "path_info.h"
#include "pimpl.h"
#include "proficiency.h"
#include "skill.h"
#include "string_formatter.h"
#include "translation.h"
#include "translations.h"
#include "type_id.h"
#include "weather.h"

diary_page::diary_page() = default;

std::vector<std::string> diary::get_pages_list()
{
    std::vector<std::string> result;
    result.reserve( pages.size() );
    for( std::unique_ptr<diary_page> &n : pages ) {
        result.push_back( get_diary_time_str( n->turn, n->time_acc ) );
    }
    return result;
}

int diary::set_opened_page( int pagenum )
{
    if( pagenum != opened_page ) {
        change_list.clear();
        desc_map.clear();
    }
    if( pages.empty() ) {
        opened_page = - 1;
    } else if( pagenum < 0 ) {
        opened_page = pages.size() - 1;
    } else {
        opened_page = pagenum % pages.size();
    }
    return opened_page;
}

int diary::get_opened_page_num() const
{
    return opened_page;
}

diary_page *diary::get_page_ptr( int offset )
{
    if( !pages.empty() && opened_page + offset >= 0 ) {

        return pages[opened_page + offset].get();
    }
    return nullptr;
}

void diary::add_to_change_list( const std::string &entry, const std::string &desc )
{
    if( !desc.empty() ) {
        desc_map[change_list.size()] = desc;
    }
    change_list.push_back( entry );
}

void diary::spell_changes()
{
    avatar *u = &get_avatar();
    diary_page *currpage = get_page_ptr();
    diary_page *prevpage = get_page_ptr( -1 );
    if( currpage == nullptr ) {
        return;
    }
    if( prevpage == nullptr ) {
        if( !currpage->known_spells.empty() ) {
            add_to_change_list( _( "Known spells:" ) );
            for( const std::pair<const string_id<spell_type>, int> &elem : currpage->known_spells ) {
                const spell s = u->magic->get_spell( elem.first );
                add_to_change_list( string_format( _( "%s: %d" ), s.name(), elem.second ), s.description() );
            }
            add_to_change_list( " " );
        }
    } else {
        if( !currpage->known_spells.empty() ) {
            bool flag = true;
            for( const std::pair<const string_id<spell_type>, int> &elem : currpage->known_spells ) {
                if( prevpage->known_spells.find( elem.first ) != prevpage->known_spells.end() ) {
                    const int prevlvl = prevpage->known_spells[elem.first];
                    if( elem.second != prevlvl ) {
                        if( flag ) {
                            add_to_change_list( _( "Improved/new spells: " ) );
                            flag = false;
                        }
                        const spell s = u->magic->get_spell( elem.first );
                        add_to_change_list( string_format( _( "%s: %d -> %d" ), s.name(), prevlvl, elem.second ),
                                            s.description() );
                    }
                } else {
                    if( flag ) {
                        add_to_change_list( _( "Improved/new spells: " ) );
                        flag = false;
                    }
                    const spell s = u->magic->get_spell( elem.first );
                    add_to_change_list( string_format( _( "%s: %d" ), s.name(), elem.second ), s.description() );
                }
            }
            if( !flag ) {
                add_to_change_list( " " );
            }
        }

    }
}

void diary::mission_changes()
{
    diary_page *currpage = get_page_ptr();
    diary_page *prevpage = get_page_ptr( -1 );
    if( currpage == nullptr ) {
        return;
    }
    if( prevpage == nullptr ) {
        auto add_missions = [&]( const std::string & name, const std::vector<int> *missions ) {
            if( !missions->empty() ) {
                bool flag = true;

                for( const int uid : *missions ) {
                    const mission *miss = mission::find( uid );
                    if( miss != nullptr ) {
                        if( flag ) {
                            add_to_change_list( name );
                            flag = false;
                        }
                        add_to_change_list( miss->name(), miss->get_description() );
                    }
                }
                if( !flag ) {
                    add_to_change_list( " " );
                }
            }
        };
        add_missions( _( "Active missions:" ), &currpage->mission_active );
        add_missions( _( "Completed missions:" ), &currpage->mission_completed );
        add_missions( _( "Failed missions:" ), &currpage->mission_failed );

    } else {
        auto add_missions = [&]( const std::string & name, const std::vector<int> *missions,
        const std::vector<int> *prevmissions ) {
            bool flag = true;
            for( const int uid : *missions ) {
                if( std::find( prevmissions->begin(), prevmissions->end(), uid ) == prevmissions->end() ) {
                    const mission *miss = mission::find( uid );
                    if( miss != nullptr ) {
                        if( flag ) {
                            add_to_change_list( name );
                            flag = false;
                        }
                        add_to_change_list( miss->name(), miss->get_description() );
                    }

                }
                if( !flag ) {
                    add_to_change_list( " " );
                }
            }
        };
        add_missions( _( "New missions:" ), &currpage->mission_active, &prevpage->mission_active );
        add_missions( _( "New completed missions:" ), &currpage->mission_completed,
                      &prevpage->mission_completed );
        add_missions( _( "New failed:" ), &currpage->mission_failed, &prevpage->mission_failed );

    }
}

void diary::bionic_changes()
{
    diary_page *currpage = get_page_ptr();
    diary_page *prevpage = get_page_ptr( -1 );
    if( currpage == nullptr ) {
        return;
    }
    if( prevpage == nullptr ) {
        if( !currpage->bionics.empty() ) {
            add_to_change_list( _( "Bionics" ) );
            for( const bionic_id &elem : currpage->bionics ) {
                const bionic_data &b = elem.obj();
                add_to_change_list( b.name.translated(), b.description.translated() );
            }
            add_to_change_list( " " );
        }
    } else {

        bool flag = true;
        if( !currpage->bionics.empty() ) {
            for( const bionic_id &elem : currpage->bionics ) {

                if( std::find( prevpage->bionics.begin(), prevpage->bionics.end(),
                               elem ) == prevpage->bionics.end() ) {
                    if( flag ) {
                        add_to_change_list( _( "New Bionics: " ) );
                        flag = false;
                    }
                    const bionic_data &b = elem.obj();
                    add_to_change_list( b.name.translated(), b.description.translated() );
                }
            }
            if( !flag ) {
                add_to_change_list( " " );
            }
        }

        flag = true;
        if( !prevpage->bionics.empty() ) {
            for( const bionic_id &elem : prevpage->bionics ) {
                if( std::find( currpage->bionics.begin(), currpage->bionics.end(),
                               elem ) == currpage->bionics.end() ) {
                    if( flag ) {
                        add_to_change_list( _( "Lost Bionics: " ) );
                        flag = false;
                    }
                    const bionic_data &b = elem.obj();
                    add_to_change_list( b.name.translated(), b.description.translated() );
                }
            }
            if( !flag ) {
                add_to_change_list( " " );
            }
        }
    }
}

void diary::kill_changes()
{
    diary_page *currpage = get_page_ptr();
    diary_page *prevpage = get_page_ptr( -1 );
    if( currpage == nullptr ) {
        return;
    }
    if( prevpage == nullptr ) {
        if( !currpage->kills.empty() ) {
            add_to_change_list( _( "Kills: " ) );
            for( const std::pair<const string_id<mtype>, int> &elem : currpage->kills ) {
                const mtype &m = elem.first.obj();
                nc_color color = m.color;
                std::string symbol = m.sym;
                std::string nname = m.nname( elem.second );
                add_to_change_list( string_format( "%4d ", elem.second ) + colorize( symbol,
                                    color ) + " " + colorize( nname, c_light_gray ), m.get_description() );
            }
            add_to_change_list( " " );
        }
        if( !currpage->npc_kills.empty() ) {
            add_to_change_list( _( "NPC Killed" ) );
            for( const std::string &npc_name : currpage->npc_kills ) {
                add_to_change_list( string_format( "%4d ", 1 ) + colorize( "@ " + npc_name, c_magenta ) );
            }
            add_to_change_list( " " );
        }

    } else {

        if( !currpage->kills.empty() ) {

            bool flag = true;
            for( const std::pair<const string_id<mtype>, int> &elem : currpage->kills ) {
                const mtype &m = elem.first.obj();
                nc_color color = m.color;
                std::string symbol = m.sym;
                std::string nname = m.nname( elem.second );
                int kills = elem.second;
                if( prevpage->kills.count( elem.first ) > 0 ) {
                    const int prevkills = prevpage->kills[elem.first];
                    if( kills > prevkills ) {
                        if( flag ) {
                            add_to_change_list( _( "Kills: " ) );
                            flag = false;
                        }
                        kills = kills - prevkills;
                        add_to_change_list( string_format( "%4d ", kills ) + colorize( symbol,
                                            color ) + " " + colorize( nname, c_light_gray ), m.get_description() );
                    }
                } else {
                    if( flag ) {
                        add_to_change_list( _( "Kills: " ) );
                        flag = false;
                    }
                    add_to_change_list( string_format( "%4d ", kills ) + colorize( symbol,
                                        color ) + " " + colorize( nname, c_light_gray ), m.get_description() );
                }

            }
            if( !flag ) {
                add_to_change_list( " " );
            }

        }
        if( !currpage->npc_kills.empty() ) {

            const std::vector<std::string> &prev_npc_kills = prevpage->npc_kills;

            bool flag = true;
            for( const std::string &npc_name : currpage->npc_kills ) {

                if( ( std::find( prev_npc_kills.begin(), prev_npc_kills.end(),
                                 npc_name ) == prev_npc_kills.end() ) ) {
                    if( flag ) {
                        add_to_change_list( _( "NPC Killed: " ) );
                        flag = false;
                    }
                    add_to_change_list( string_format( "%4d ", 1 ) + colorize( "@ " + npc_name, c_magenta ) );
                }

            }
            if( !flag ) {
                add_to_change_list( " " );
            }
        }
    }
}

void diary::skill_changes()
{
    diary_page *currpage = get_page_ptr();
    diary_page *prevpage = get_page_ptr( -1 );
    if( currpage == nullptr ) {
        return;
    }
    if( prevpage == nullptr ) {
        if( currpage->skillsL.empty() ) {
            return;
        } else {

            add_to_change_list( _( "Skills:" ) );
            for( const std::pair<const string_id<Skill>, int> &elem : currpage->skillsL ) {

                if( elem.second > 0 ) {
                    Skill s = elem.first.obj();
                    add_to_change_list( string_format( "<color_light_blue>%s: %d</color>", s.name(), elem.second ),
                                        s.description() );
                }
            }
            add_to_change_list( "" );
        }
    } else {

        bool flag = true;
        for( const std::pair<const string_id<Skill>, int> &elem : currpage->skillsL ) {
            if( prevpage->skillsL.find( elem.first ) != prevpage->skillsL.end() ) {
                if( prevpage->skillsL[elem.first] != elem.second ) {
                    if( flag ) {
                        add_to_change_list( _( "Skills: " ) );
                        flag = false;
                    }
                    Skill s = elem.first.obj();
                    add_to_change_list( string_format( _( "<color_light_blue>%s: %d -> %d</color>" ), s.name(),
                                                       prevpage->skillsL[elem.first], elem.second ), s.description() );
                }

            }

        }
        if( !flag ) {
            add_to_change_list( " " );
        }

    }
}

void diary::trait_changes()
{
    diary_page *currpage = get_page_ptr();
    diary_page *prevpage = get_page_ptr( -1 );
    if( currpage == nullptr ) {
        return;
    }
    if( prevpage == nullptr ) {
        if( !currpage->traits.empty() ) {
            add_to_change_list( _( "Mutations:" ) );
            for( const trait_and_var &elem : currpage->traits ) {
                add_to_change_list( colorize( elem.name(), elem.trait->get_display_color() ), elem.desc() );
            }
            add_to_change_list( "" );
        }
    } else {
        if( prevpage->traits.empty() && !currpage->traits.empty() ) {
            add_to_change_list( _( "Mutations:" ) );
            for( const trait_and_var &elem : currpage->traits ) {
                add_to_change_list( colorize( elem.name(), elem.trait->get_display_color() ), elem.desc() );

            }
            add_to_change_list( "" );
        } else {

            bool flag = true;
            for( const trait_and_var &elem : currpage->traits ) {

                if( std::find( prevpage->traits.begin(), prevpage->traits.end(),
                               elem ) == prevpage->traits.end() ) {
                    if( flag ) {
                        add_to_change_list( _( "Gained Mutation: " ) );
                        flag = false;
                    }
                    add_to_change_list( colorize( elem.name(), elem.trait->get_display_color() ), elem.desc() );
                }

            }
            if( !flag ) {
                add_to_change_list( " " );
            }

            flag = true;
            for( const trait_and_var &elem : prevpage->traits ) {

                if( std::find( currpage->traits.begin(), currpage->traits.end(),
                               elem ) == currpage->traits.end() ) {
                    if( flag ) {
                        add_to_change_list( _( "Lost Mutation: " ) );
                        flag = false;
                    }
                    add_to_change_list( colorize( elem.name(), elem.trait->get_display_color() ), elem.desc() );
                }
            }
            if( !flag ) {
                add_to_change_list( " " );
            }

        }
    }
}

void diary::stat_changes()
{
    diary_page *currpage = get_page_ptr();
    diary_page *prevpage = get_page_ptr( -1 );
    if( currpage == nullptr ) {
        return;
    }
    if( prevpage == nullptr ) {
        add_to_change_list( _( "Stats:" ) );
        add_to_change_list( string_format( _( "Strength: %d" ), currpage->strength ) );
        add_to_change_list( string_format( _( "Dexterity: %d" ), currpage->dexterity ) );
        add_to_change_list( string_format( _( "Intelligence: %d" ), currpage->intelligence ) );
        add_to_change_list( string_format( _( "Perception: %d" ), currpage->perception ) );
        add_to_change_list( " " );
    } else {

        bool flag = true;
        if( currpage->strength != prevpage->strength ) {
            if( flag ) {
                add_to_change_list( _( "Stats: " ) );
                flag = false;
            }
            add_to_change_list( string_format( _( "Strength: %d -> %d" ), prevpage->strength,
                                               currpage->strength ) );
        }
        if( currpage->dexterity != prevpage->dexterity ) {
            if( flag ) {
                add_to_change_list( _( "Stats: " ) );
                flag = false;
            }
            add_to_change_list( string_format( _( "Dexterity: %d -> %d" ), prevpage->dexterity,
                                               currpage->dexterity ) );
        }
        if( currpage->intelligence != prevpage->intelligence ) {
            if( flag ) {
                add_to_change_list( _( "Stats: " ) );
                flag = false;
            }
            add_to_change_list( string_format( _( "Intelligence: %d -> %d" ), prevpage->intelligence,
                                               currpage->intelligence ) );
        }

        if( currpage->perception != prevpage->perception ) {
            if( flag ) {
                add_to_change_list( _( "Stats: " ) );
                flag = false;
            }
            add_to_change_list( string_format( _( "Perception: %d -> %d" ), prevpage->perception,
                                               currpage->perception ) );
        }
        if( !flag ) {
            add_to_change_list( " " );
        }

    }
}

void diary::prof_changes()
{
    diary_page *currpage = get_page_ptr();
    diary_page *prevpage = get_page_ptr( -1 );
    if( currpage == nullptr ) {
        return;
    }
    if( prevpage == nullptr ) {
        if( !currpage->known_profs.empty() ) {
            add_to_change_list( _( "Proficiencies:" ) );
            for( const proficiency_id &elem : currpage->known_profs ) {
                const proficiency &p = elem.obj();
                add_to_change_list( p.name(), p.description() );
            }
            add_to_change_list( "" );
        }

    } else {

        bool flag = true;
        for( const proficiency_id &elem : currpage->known_profs ) {

            if( std::find( prevpage->known_profs.begin(), prevpage->known_profs.end(),
                           elem ) == prevpage->known_profs.end() ) {
                if( flag ) {
                    add_to_change_list( _( "Proficiencies: " ) );
                    flag = false;
                }
                const proficiency &p = elem.obj();
                add_to_change_list( p.name(), p.description() );
            }
        }
        if( !flag ) {
            add_to_change_list( " " );
        }
    }
}

std::vector<std::string> diary::get_change_list()
{
    if( !change_list.empty() ) {
        return change_list;
    }
    if( !pages.empty() ) {
        stat_changes();
        skill_changes();
        prof_changes();
        trait_changes();
        bionic_changes();
        spell_changes();
        mission_changes();
        kill_changes();
    }
    return change_list;
}

std::map<int, std::string> diary::get_desc_map()
{
    if( !desc_map.empty() ) {
        return desc_map;
    } else {
        get_change_list();
        return desc_map;
    }
}

std::string diary::get_page_text()
{

    if( !pages.empty() ) {
        return get_page_ptr()->m_text;
    }
    return "";
}

std::string diary::get_head_text()
{

    if( !pages.empty() ) {
        const diary_page *prevpageptr = get_page_ptr( -1 );
        const diary_page *currpageptr = get_page_ptr();
        const time_point prev_turn = ( prevpageptr != nullptr ) ? prevpageptr->turn : calendar::turn_zero;
        const time_duration turn_diff = currpageptr->turn - prev_turn;
        std::string time_diff_text;
        if( opened_page != 0 ) {
            time_diff_text = get_diary_time_since_str( turn_diff, currpageptr->time_acc );
        }
        //~ Head text of a diary page
        //~ %1$d is the current page number, %2$d is the number of pages in total
        //~ %3$s is the time point when the current page was created
        //~ %4$s is time relative to the previous page
        return string_format( _( "Entry: %1$d/%2$d, %3$s, %4$s" ),
                              opened_page + 1, pages.size(),
                              get_diary_time_str( currpageptr->turn, currpageptr->time_acc ),
                              time_diff_text );
    }
    return "";
}

void diary::death_entry()
{
    bool lasttime = query_yn( _( "Open diary for the last time?" ) );
    if( lasttime ) {
        show_diary_ui( this );
    }
    export_to_txt( true );
}

diary::diary()
{
    owner = get_avatar().name;
}
void diary::set_page_text( std::string text )
{
    get_page_ptr()->m_text = std::move( text );
}

void diary::new_page()
{
    std::unique_ptr<diary_page> page( new diary_page() );
    page -> m_text = std::string();
    page -> turn = calendar::turn;
    page -> kills = g ->get_kill_tracker().kills;
    page -> npc_kills = g->get_kill_tracker().npc_kills;
    avatar *u = &get_avatar();
    page -> time_acc = u->has_watch() ? time_accuracy::FULL :
                       is_creature_outside( *u ) ? time_accuracy::PARTIAL : time_accuracy::NONE;
    page -> mission_completed = mission::to_uid_vector( u->get_completed_missions() );
    page -> mission_active = mission::to_uid_vector( u->get_active_missions() );
    page -> mission_failed = mission::to_uid_vector( u->get_failed_missions() );
    page -> male = u->male;
    page->strength = u->get_str_base();
    page->dexterity = u->get_dex_base();
    page->intelligence = u->get_int_base();
    page->perception = u->get_per_base();
    page->traits = u->get_mutations_variants( false );
    const auto spells = u->magic->get_spells();
    for( const spell *spell : spells ) {
        const spell_id &id = spell->id();
        const int lvl = spell->get_level();
        page-> known_spells[id] = lvl;
    }
    page-> bionics = u->get_bionics();
    for( Skill &elem : Skill::skills ) {

        int level = u->get_skill_level_object( elem.ident() ).level();
        page->skillsL.insert( { elem.ident(), level } );
    }
    page -> known_profs = u->_proficiencies->known_profs();
    page -> learning_profs = u->_proficiencies->learning_profs();
    page->max_power_level = u->get_max_power_level();
    diary::pages.push_back( std::move( page ) );
}

void diary::delete_page()
{
    if( opened_page < static_cast<int>( pages.size() ) ) {
        pages.erase( pages.begin() + opened_page );
        set_opened_page( opened_page - 1 );
    }
}

void diary::export_to_txt( bool lastexport )
{
    std::ofstream myfile;
    cata_path path = lastexport ? PATH_INFO::memorialdir_path() :
                     PATH_INFO::world_base_save_path();
    path = path / ( owner + "s_diary.txt" );
    myfile.open( path.get_unrelative_path() );

    for( int i = 0; i < static_cast<int>( pages.size() ); i++ ) {
        set_opened_page( i );
        const diary_page page = *get_page_ptr();
        myfile << get_head_text() + "\n\n";
        for( std::string &str : this->get_change_list() ) {
            myfile << remove_color_tags( str ) + "\n";
        }
        std::vector<std::string> folded_text = foldstring( page.m_text, 50 );
        for( const std::string &line : folded_text ) {
            myfile << line + "\n";
        }
        myfile <<  "\n\n\n";
    }
    myfile.close();
}

bool diary::store()
{
    std::string name = base64_encode( get_avatar().get_save_id() + "_diary" );
    cata_path path = PATH_INFO::world_base_save_path() / ( name + ".json" );
    const bool iswriten = write_to_file( path, [&]( std::ostream & fout ) {
        serialize( fout );
    }, _( "diary data" ) );
    return iswriten;
}

void diary::serialize( std::ostream &fout )
{
    JsonOut jsout( fout, true );
    jsout.start_object();
    serialize( jsout );
    jsout.end_object();
}

void diary::serialize( JsonOut &jsout )
{
    jsout.member( "owner", owner );
    jsout.member( "pages" );
    jsout.start_array();
    for( std::unique_ptr<diary_page> &n : pages ) {
        jsout.start_object();
        jsout.member( "text", n->m_text );
        jsout.member( "turn", n->turn );
        jsout.member( "time_accuracy", n->time_acc );
        jsout.member( "completed", n->mission_completed );
        jsout.member( "active", n->mission_active );
        // TODO: migrate "faild" to "failed"?
        jsout.member( "faild", n->mission_failed );
        jsout.member( "kills", n->kills );
        jsout.member( "npc_kills", n->npc_kills );
        jsout.member( "male", n->male );
        jsout.member( "str", n->strength );
        jsout.member( "dex", n->dexterity );
        jsout.member( "int", n->intelligence );
        jsout.member( "per", n->perception );
        jsout.member( "traits", n->traits );
        jsout.member( "bionics", n->bionics );
        jsout.member( "spells", n->known_spells );
        jsout.member( "skillsL", n->skillsL );
        jsout.member( "known_profs", n->known_profs );
        jsout.member( "learning_profs", n->learning_profs );
        jsout.member( "max_power_level", n->max_power_level );
        jsout.end_object();
    }
    jsout.end_array();
}

void diary::load()
{
    std::string name = base64_encode( get_avatar().get_save_id() + "_diary" );
    cata_path path = PATH_INFO::world_base_save_path() / ( name + ".json" );
    if( file_exist( path ) ) {
        read_from_file_json( path, [&]( const JsonValue & jv ) {
            deserialize( jv );
        } );
    }
}

void diary::deserialize( const JsonValue &jsin )
{
    try {
        JsonObject data = jsin.get_object();

        data.read( "owner", owner );
        pages.clear();
        for( JsonObject elem : data.get_array( "pages" ) ) {
            std::unique_ptr<diary_page> page( new diary_page() );
            page->m_text = elem.get_string( "text" );
            elem.read( "turn", page->turn );
            elem.read( "time_accuracy", page->time_acc );
            elem.read( "active", page->mission_active );
            elem.read( "completed", page->mission_completed );
            // TODO: migrate "faild" to "failed"?
            elem.read( "faild", page->mission_failed );
            elem.read( "kills", page->kills );
            elem.read( "npc_kills", page->npc_kills );
            elem.read( "male", page->male );
            elem.read( "str", page->strength );
            elem.read( "dex", page->dexterity );
            elem.read( "int", page->intelligence );
            elem.read( "per", page->perception );
            elem.read( "traits", page->traits );
            elem.read( "bionics", page->bionics );
            elem.read( "spells", page->known_spells );
            elem.read( "skillsL", page->skillsL );
            elem.read( "known_profs", page->known_profs );
            elem.read( "learning_profs", page->learning_profs );
            elem.read( "max_power_level", page->max_power_level );
            diary::pages.push_back( std::move( page ) );
        }
    } catch( const JsonError &e ) {

    }
}
