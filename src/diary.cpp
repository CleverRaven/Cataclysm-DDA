#include "diary.h"

#include <algorithm>
#include <array>
#include <fstream>
#include <optional>
#include <string>
#include <tuple>
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

std::string diary_page::entry_name() const
{
    return get_diary_time_str( turn, time_acc );
}

std::string diary_page_summary::entry_name() const
{
    return _( "Summary" );
}

using diary_entry_opt = std::optional<std::pair<std::string, std::string>>;

std::vector<std::string> diary::get_pages_list() const
{
    std::vector<std::string> result;
    result.reserve( pages.size() );
    for( const std::unique_ptr<diary_page> &n : pages ) {
        result.push_back( n->entry_name() );
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


diary_page *diary::get_page_ptr( int offset, bool allow_summary )
{
    const int page = offset + opened_page;
    if( !pages.empty() && page >= 0 && page < static_cast<int>( pages.size() ) ) {
        diary_page *const ptr = pages[page].get();
        if( allow_summary || !ptr->is_summary() ) {
            return ptr;
        }
    }
    return nullptr;
}

size_t diary::add_to_change_list( const std::string &entry, const std::string &desc )
{
    const size_t index = change_list.size();
    if( !desc.empty() ) {
        desc_map[index] = desc;
    }
    change_list.push_back( entry );
    return index;
}

void diary::update_change_list( const size_t index, const std::string &entry,
                                const std::string &desc )
{
    if( index >= change_list.size() ) {
        return;
    }
    if( desc.empty() ) {
        desc_map.erase( index );
    } else {
        desc_map[index] = desc;
    }
    change_list[index] = entry;
}

void diary::open_summary_page()
{
    add_to_change_list( _( "It is currently:" ) );
    add_to_change_list( get_diary_time_str( calendar::turn, time_acc() ) );
    add_to_change_list( "" );
    add_to_change_list( _( "You have survived:" ) );
    add_to_change_list( get_diary_time_since_str( calendar::turn - calendar::start_of_cataclysm,
                        time_acc(), false ) );
    add_to_change_list( _( "You have been playing for:" ) );
    add_to_change_list( get_diary_time_since_str( calendar::turn - calendar::start_of_game,
                        time_acc(), false ) );
    add_to_change_list( "" );
    const std::string season_name = calendar::name_season( season_of_year( calendar::turn ) );
    add_to_change_list( string_format( _( "It is currently %s." ), season_name ) );
    add_to_change_list( string_format( _( "%s will last for %d more days." ), season_name,
                                       to_days<int>( calendar::season_length() ) - day_of_season<int>( calendar::turn ) ) );
}

template<typename Container, typename Fn>
void diary::changes( Container diary_page::* member, Fn &&get_entry,
                     // NOLINTNEXTLINE(cata-use-string_view)
                     const std::string &heading_first, const std::string &headings_first,
                     const std::string &heading, const std::string &headings,
                     diary_page *const currpage, diary_page *const prevpage )
{
    if( !currpage ) {
        return;
    }
    size_t heading_index;
    int count = 0;
    for( const auto &elem : currpage->*member ) {
        const diary_entry_opt entry = get_entry( elem, prevpage );
        if( entry ) {
            if( count == 0 ) {
                heading_index = add_to_change_list( prevpage ? heading : heading_first );
            }
            add_to_change_list( entry->first, entry->second );
            count++;
        }
    }
    if( count > 0 ) {
        if( count > 1 ) {
            update_change_list( heading_index, prevpage ? headings : headings_first );
        }
        add_to_change_list( " " );
    }
}

template<typename Container, typename Fn>
void diary::changes( Container diary_page::* member, Fn &&get_entry,
                     // NOLINTNEXTLINE(cata-use-string_view)
                     const std::string &heading_first, const std::string &headings_first,
                     const std::string &heading, const std::string &headings )
{
    changes( member, get_entry, heading_first, headings_first, heading, headings, get_page_ptr(),
             get_page_ptr( -1, false ) );
}

template<typename Container, typename Fn>
void diary::changes( Container diary_page::* member, Fn &&get_entry,
                     // NOLINTNEXTLINE(cata-use-string_view)
                     const std::string &heading_first, const std::string &headings_first )
{
    changes( member, get_entry, heading_first, headings_first, heading_first, headings_first );
}

static diary_entry_opt spell_change( const std::pair<const string_id<spell_type>, int> &elem,
                                     diary_page *const prevpage )
{
    avatar *u = &get_avatar();
    const bool improved = prevpage && prevpage->known_spells.count( elem.first ) > 0;
    const int prevlvl = improved ? prevpage->known_spells[ elem.first ] : -1;
    if( elem.second != prevlvl ) {
        const spell s = u->magic->get_spell( elem.first );
        return { {
                improved ?
                string_format( _( "%s: %d -> %d" ), s.name(), prevlvl, elem.second ) :
                string_format( _( "%s: %d" ), s.name(), elem.second )
                , s.description()
            } };

    }
    return std::nullopt;
}

void diary::spell_changes()
{
    changes( &diary_page::known_spells,
             spell_change,
             _( "Known spell: " ), _( "Known spells: " ),
             _( "Improved/new spell: " ), _( "Improved/new spells: " )
           );
}

static diary_entry_opt mission_change( const int &elem, diary_page *const prevpage )
{
    if( !prevpage ||
        std::find( prevpage->mission_active.begin(), prevpage->mission_active.end(),
                   elem ) != prevpage->mission_active.end() ) {
        const mission *miss = mission::find( elem );
        if( miss != nullptr ) {
            return { { miss->name(), miss->get_description() } };
        }
    }
    return std::nullopt;
}

void diary::mission_changes()
{
    changes( &diary_page::mission_active,
             mission_change,
             _( "Active mission: " ), _( "Active missions: " ),
             _( "New mission: " ), _( "New missions: " )
           );
    changes( &diary_page::mission_completed,
             mission_change,
             _( "Completed mission: " ), _( "Completed missions: " ),
             _( "New completed mission: " ), _( "New completed missions: " )
           );
    changes( &diary_page::mission_failed,
             mission_change,
             _( "Failed mission: " ), _( "Failed missions: " ),
             _( "New failed mission: " ), _( "New failed missions: " )
           );
}

static diary_entry_opt bionic_change( const bionic_id &elem, diary_page *const prevpage )
{
    if( !prevpage ||
        std::find( prevpage->bionics.begin(), prevpage->bionics.end(),
                   elem ) != prevpage->bionics.end() ) {
        const bionic_data &b = elem.obj();
        return { { b.name.translated(), b.description.translated() } };
    }
    return std::nullopt;
}

void diary::bionic_changes()
{
    changes( &diary_page::bionics, bionic_change, _( "New Bionic: " ), _( "New Bionics: " ) );
    changes( &diary_page::bionics,
             bionic_change,
             _( "Lost Bionic: " ), _( "Lost Bionics: " ),
             _( "Lost Bionic: " ), _( "Lost Bionics: " ),
             get_page_ptr( -1, false ), get_page_ptr()
           );
}

static diary_entry_opt mon_kill_change( const std::pair<const string_id<mtype>, int> &elem,
                                        diary_page *const prevpage )
{
    const mtype &m = elem.first.obj();
    nc_color color = m.color;
    std::string symbol = m.sym;
    std::string nname = m.nname( elem.second );
    int kills = elem.second;
    const int prevkills = prevpage ? prevpage->kills[ elem.first ] : 0;
    if( kills > prevkills ) {
        return { {
                string_format( "%4d ", kills - prevkills ) +
                colorize( symbol, color ) + " " +
                colorize( nname, c_light_gray ), m.get_description()
            } };
    }
    return std::nullopt;
}

void diary::mon_kill_changes()
{
    changes( &diary_page::kills, mon_kill_change, _( "Kill: " ), _( "Kills: " ) );
}

static diary_entry_opt npc_kill_change( const std::string &npc_name, diary_page *const prevpage )
{
    if( !prevpage || ( std::find( prevpage->npc_kills.begin(), prevpage->npc_kills.end(),
                                  npc_name ) == prevpage->npc_kills.end() ) ) {
        return { { "   1 " + colorize( "@ " + npc_name, c_magenta ), "" } };
    }
    return std::nullopt;
}

void diary::npc_kill_changes()
{
    changes( &diary_page::npc_kills, npc_kill_change, _( "NPC killed: " ), _( "NPCs killed: " ) );
}

static diary_entry_opt skill_change( const std::pair<const skill_id, int> &elem,
                                     diary_page *const prevpage )
{
    const int prevlvl = prevpage ? prevpage->skillsL[ elem.first ] : 0;
    if( elem.second > prevlvl ) {
        const Skill &s = elem.first.obj();
        return { {
                prevpage ?
                string_format( _( "<color_light_blue>%s: %d -> %d</color>" ), s.name(),
                               prevpage->skillsL[ elem.first ], elem.second ) :
                string_format( "<color_light_blue>%s: %d</color>", s.name(), elem.second ),
                s.description()
            } };
    }
    return std::nullopt;
}

void diary::skill_changes()
{
    changes( &diary_page::skillsL, skill_change, _( "Skills: " ), _( "Skills: " ) );
}

static diary_entry_opt trait_change( const trait_and_var &elem, diary_page *const prevpage )
{
    if( !prevpage ||
        std::find( prevpage->traits.begin(), prevpage->traits.end(), elem ) == prevpage->traits.end() ) {
        return { { colorize( elem.name(), elem.trait->get_display_color() ), elem.desc() } };
    }
    return std::nullopt;
}

void diary::trait_changes()
{
    changes( &diary_page::traits,
             trait_change,
             _( "Mutation: " ), _( "Mutations: " ),
             _( "Gained mutation: " ), _( "Gained mutations: " )
           );
    changes( &diary_page::traits,
             trait_change,
             _( "Lost mutation: " ), _( "Lost mutation: " ),
             _( "Lost mutation: " ), _( "Lost mutations: " ),
             get_page_ptr( -1, false ), get_page_ptr()
           );
}

static const std::array<std::tuple<translation, translation, int diary_page::*>, 4> diary_stats
= { {
        { to_translation( "Strength: %d" ),     to_translation( "Strength: %d -> %d" ),     &diary_page::strength },
        { to_translation( "Dexterity: %d" ),    to_translation( "Dexterity: %d -> %d" ),    &diary_page::dexterity },
        { to_translation( "Intelligence: %d" ), to_translation( "Intelligence: %d -> %d" ), &diary_page::intelligence },
        { to_translation( "Perception: %d" ),   to_translation( "Perception: %d -> %d" ),   &diary_page::perception }
    }
};

void diary::stat_changes()
{
    const diary_page *const currpage = get_page_ptr();
    const diary_page *const prevpage = get_page_ptr( -1, false );
    if( currpage == nullptr ) {
        return;
    }
    bool flag = true;
    for( const auto &stat : diary_stats ) {
        if( flag ) {
            add_to_change_list( _( "Stats: " ) );
            flag = false;
        }
        const int prev = prevpage ? prevpage->*std::get<2>( stat ) : -1;
        const int curr = currpage->*std::get<2>( stat );
        if( !prevpage || prev == curr ) {
            add_to_change_list( string_format( std::get<0>( stat ), curr ) );
        } else {
            add_to_change_list( string_format( std::get<1>( stat ), prev, curr ) );
        }
    }
    if( !flag ) {
        add_to_change_list( " " );
    }
}

static diary_entry_opt prof_change( const proficiency_id &elem, diary_page *const prevpage )
{
    if( !prevpage ||
        std::find( prevpage->known_profs.begin(), prevpage->known_profs.end(),
                   elem ) == prevpage->known_profs.end() ) {
        const proficiency &p = elem.obj();
        return { { p.name(), p.description() } };
    }
    return std::nullopt;
}


void diary::prof_changes()
{
    changes( &diary_page::known_profs,
             prof_change,
             _( "Proficiency: " ), _( "Proficiencies: " ),
             _( "Gained proficiency: " ), _( "Gained proficiencies: " )
           );
}

const std::vector<std::string> &diary::get_change_list()
{
    if( !change_list.empty() || pages.empty() ) {
        return change_list;
    }
    if( get_page_ptr()->is_summary() ) {
        open_summary_page();
    } else {
        stat_changes();
        skill_changes();
        prof_changes();
        trait_changes();
        bionic_changes();
        spell_changes();
        mission_changes();
        mon_kill_changes();
        npc_kill_changes();
    }
    while( !change_list.empty() && change_list.back() == " " ) {
        // usually one, possibly more or none, changes functions will append a trailing blank line.
        change_list.pop_back();
    }
    return change_list;
}

std::map<int, std::string> &diary::get_desc_map()
{
    if( desc_map.empty() ) {
        get_change_list();
    }
    return desc_map;
}

std::string diary::get_page_text()
{

    if( !pages.empty() ) {
        return get_page_ptr()->m_text;
    }
    return "";
}

std::string diary::get_head_text( bool is_summary )
{

    if( !pages.empty() ) {
        const diary_page *prevpageptr = get_page_ptr( -1, false );
        const diary_page *currpageptr = get_page_ptr();
        const time_point prev_turn = ( prevpageptr != nullptr ) ? prevpageptr->turn :
                                     calendar::start_of_game;
        const time_duration turn_diff = currpageptr->turn - prev_turn;
        std::string time_diff_text;
        if( prevpageptr ) {
            time_diff_text = get_diary_time_since_str( turn_diff, currpageptr->time_acc );
        }
        if( is_summary ) {
            //~ Head text of diary summary page
            //~ %1$d is the number of entries in total
            return string_format( _( "Summary: %1$d entries" ), pages.size() - 1 );
        } else if( time_diff_text.empty() ) {
            //~ Head text of first diary entry
            //~ %1$d is the current entry number, %2$d is the number of entries in total
            //~ %3$s is the time point when the current entry was created
            return string_format( _( "Entry: %1$d/%2$d, %3$s" ),
                                  opened_page, pages.size() - 1,
                                  get_diary_time_str( currpageptr->turn, currpageptr->time_acc ) );
        } else {
            //~ Head text of second and later diary entries
            //~ %1$d is the current entry number, %2$d is the number of entries in total
            //~ %3$s is the time point when the current entry was created
            //~ %4$s is time relative to the previous entry
            return string_format( _( "Entry: %1$d/%2$d, %3$s, %4$s" ),
                                  opened_page, pages.size() - 1,
                                  get_diary_time_str( currpageptr->turn, currpageptr->time_acc ),
                                  time_diff_text );
        }
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

void diary::add_summary_page()
{
    std::unique_ptr<diary_page> summary( new diary_page_summary() );
    summary->turn = calendar::start_of_cataclysm;
    summary->time_acc = time_accuracy::FULL;
    diary::pages.push_back( std::move( summary ) );
}

diary::diary()
{
    owner = get_avatar().name;
    add_summary_page();
}

void diary::set_page_text( std::string text )
{
    get_page_ptr()->m_text = std::move( text );
}

time_accuracy diary::time_acc() const
{
    avatar *u = &get_avatar();
    return u->has_watch() ? time_accuracy::FULL : is_creature_outside( *u ) ? time_accuracy::PARTIAL :
           time_accuracy::NONE;
}

void diary::new_page()
{
    std::unique_ptr<diary_page> page( new diary_page() );
    page -> m_text = std::string();
    page -> turn = calendar::turn;
    page -> kills = g ->get_kill_tracker().kills;
    page -> npc_kills = g->get_kill_tracker().npc_kills;
    avatar *u = &get_avatar();
    page -> time_acc = time_acc();
    page -> mission_completed = mission::to_uid_vector( u->get_completed_missions() );
    page -> mission_active = mission::to_uid_vector( u->get_active_missions() );
    page -> mission_failed = mission::to_uid_vector( u->get_failed_missions() );
    page -> male = u->male;
    page->strength = u->get_str_base();
    page->dexterity = u->get_dex_base();
    page->intelligence = u->get_int_base();
    page->perception = u->get_per_base();
    page->traits = u->get_mutations_variants( false );
    std::sort( page->traits.begin(), page->traits.end(), trait_var_display_sort );
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
    std::sort( page->known_profs.begin(), page->known_profs.end() );
    page -> learning_profs = u->_proficiencies->learning_profs();
    std::sort( page->learning_profs.begin(), page->learning_profs.end() );
    page->max_power_level = u->get_max_power_level();
    diary::pages.push_back( std::move( page ) );
}

void diary::delete_page()
{
    if( opened_page > 0 && opened_page < static_cast<int>( pages.size() ) ) {
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
        myfile << get_head_text( page.is_summary() ) + "\n\n";
        for( const std::string &str : this->get_change_list() ) {
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

bool diary::store() const
{
    std::string name = base64_encode( get_avatar().get_save_id() + "_diary" );
    cata_path path = PATH_INFO::world_base_save_path() / ( name + ".json" );
    const bool iswriten = write_to_file( path, [&]( std::ostream & fout ) {
        serialize( fout );
    }, _( "diary data" ) );
    return iswriten;
}

void diary::serialize( std::ostream &fout ) const
{
    JsonOut jsout( fout, true );
    jsout.start_object();
    serialize( jsout );
    jsout.end_object();
}

void diary::serialize( JsonOut &jsout ) const
{
    jsout.member( "owner", owner );
    jsout.member( "pages" );
    jsout.start_array();
    for( const std::unique_ptr<diary_page> &n : pages ) {
        if( n->is_summary() ) {
            continue;
        }
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
        add_summary_page();
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
