#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iterator>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "bionics.h"
#include "bodypart.h"
#include "calendar.h"
#include "cata_imgui.h"
#include "character.h"
#include "color.h"
#include "coordinates.h"
#include "creature.h"
#include "debug.h"
#include "debug_menu.h"
#include "effect.h"
#include "enums.h"
#include "game.h"
#include "game_constants.h"
#include "imgui/imgui.h"
#include "input.h"
#include "input_context.h"
#include "input_enums.h"
#include "input_popup.h"
#include "item.h"
#include "item_factory.h"
#include "item_group.h"
#include "itype.h"
#include "localized_comparator.h"
#include "map.h"
#include "memory_fast.h"
#include "mongroup.h"
#include "monster.h"
#include "monstergenerator.h"
#include "mtype.h"
#include "mutation.h"
#include "output.h"
#include "overmap.h"
#include "overmapbuffer.h"
#include "pimpl.h"
#include "point.h"
#include "proficiency.h"
#include "skill.h"
#include "string_formatter.h"
#include "string_input_popup.h"
#include "text_snippets.h"
#include "translation.h"
#include "translations.h"
#include "type_id.h"
#include "uilist.h"
#include "uistate.h"
#include "units.h"
#include "value_ptr.h"

class uilist_impl;

static const efftype_id effect_pet( "pet" );

static const mongroup_id GROUP_ZOMBIE( "GROUP_ZOMBIE" );

class wish_mutate_callback: public uilist_callback
{
    public:
        // Last menu entry
        int lastlen = 0;
        // Feedback message
        std::string msg;
        bool started = false;
        bool only_active = false;
        std::vector<trait_id> vTraits;
        std::map<trait_id, bool> pTraits;
        Character *you;

        nc_color mcolor( const trait_id &m ) {
            if( pTraits[ m ] ) {
                return c_green;
            }
            return c_white;
        }

        template<typename T>
        void ValueRow( const char *key, const std::vector<T> &list ) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted( key );
            ImGui::TableNextColumn();
            for( const auto &value : list ) {
                auto color = mcolor( value );
                ImGui::TextColored( color,
                                    "%s",
                                    value->name() );
            }
            ImGui::NewLine();
        }

        void ValueRow( const char *key, const std::vector<string_id<mutation_branch>> &list ) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted( key );
            ImGui::TableNextColumn();
            for( const auto &value : list ) {
                nc_color color = mcolor( value );
                ImGui::TextColored( color,
                                    "%s",
                                    value->name().c_str() );
            }
            ImGui::NewLine();
        }

        void ValueRow( const char *key, const std::vector<mutation_category_id> &list ) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted( key );
            ImGui::TableNextColumn();
            for( const auto &value : list ) {
                ImGui::TextColored( c_light_gray, "%s", value.c_str() );
            }
            ImGui::NewLine();
        }

        void ValueRow( const char *key, const std::set<std::string> &list ) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted( key );
            ImGui::TableNextColumn();
            for( const auto &value : list ) {
                ImGui::TextColored( c_light_gray, "%s", value.c_str() );
            }
            ImGui::NewLine();
        }

        void ValueRow( const char *key, const char *value ) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted( key );
            ImGui::TableNextColumn();
            ImGui::TextColored( c_light_gray, "%s", value );
        }

        wish_mutate_callback() = default;
        bool key( const input_context &, const input_event &event, int entnum, uilist *menu ) override {
            if( event.get_first_input() == 't' && you->has_trait( vTraits[ entnum ] ) ) {
                if( !you->has_base_trait( vTraits[ entnum ] ) ) {
                    you->unset_mutation( vTraits[ entnum ] );
                }

                you->toggle_trait( vTraits[ entnum ] );
                you->set_mutation( vTraits[ entnum ] );

                menu->entries[ entnum ].text_color = you->has_trait( vTraits[ entnum ] ) ? c_green :
                                                     menu->text_color;
                menu->entries[ entnum ].extratxt.txt = you->has_base_trait( vTraits[ entnum ] ) ? "T" : "";
                return true;
            } else if( event.get_first_input() == 'a' ) {
                only_active = !only_active;

                for( size_t i = 0; i < vTraits.size(); i++ ) {
                    if( !you->has_trait( vTraits[ i ] ) ) {
                        menu->entries[ i ].enabled = !only_active;
                    }
                }

                return true;
            }
            return false;
        }

        float desired_extra_space_right( ) override {
            return 40 * ImGui::CalcTextSize( "X" ).x;
        }

        void refresh( uilist *menu ) override {
            if( !started ) {
                started = true;
                for( const mutation_branch &traits_iter : mutation_branch::get_all() ) {
                    vTraits.push_back( traits_iter.id );
                    pTraits[traits_iter.id] = you->has_trait( traits_iter.id );
                }
            }

            ImGui::TableSetColumnIndex( 2 );

            if( menu->previewing >= 0 && static_cast<size_t>( menu->previewing ) < vTraits.size() ) {
                const mutation_branch &mdata = vTraits[menu->previewing].obj();

                ImGui::TextUnformatted( mdata.valid ? _( "Valid" ) : _( "Nonvalid" ) );
                ImGui::NewLine();

                if( ImGui::BeginTable( "props", 2 ) ) {
                    ImGui::TableSetupColumn( "key" );
                    ImGui::TableSetupColumn( "value" );
                    ValueRow( _( "Id:" ), mdata.id.c_str() );
                    ValueRow( _( "Prereqs:" ), mdata.prereqs );
                    if( !mdata.prereqs2.empty() ) {
                        ValueRow( _( "Prereqs, 2d:" ), mdata.prereqs2 );
                    }
                    if( !mdata.threshreq.empty() ) {
                        ValueRow( _( "Thresholds required:" ), mdata.threshreq );
                    }
                    if( !mdata.cancels.empty() ) {
                        ValueRow( _( "Cancels:" ), mdata.cancels );
                    }
                    if( !mdata.replacements.empty() ) {
                        ValueRow( _( "Becomes:" ), mdata.replacements );
                    }
                    if( !mdata.additions.empty() ) {
                        ValueRow( _( "Add-ons:" ), mdata.additions );
                    }
                    if( !mdata.types.empty() ) {
                        ValueRow( _( "Type:" ), mdata.types );
                    }
                    if( !mdata.category.empty() ) {
                        ValueRow( _( "Category:" ), mdata.category );
                    }
                    ImGui::EndTable();
                }
                ImGui::NewLine();

                //~ pts: points, vis: visibility, ugly: ugliness
                ImGui::Text( _( "pts: %d vis: %d ugly: %d" ),
                             mdata.points,
                             mdata.visibility,
                             mdata.ugliness );
                ImGui::NewLine();
                ImGui::TextWrapped( "%s", you->mutation_desc( mdata.id ).c_str() );
            }

            float y = ImGui::GetContentRegionMax().y - 3 * ImGui::GetTextLineHeightWithSpacing();
            if( ImGui::GetCursorPosY() < y ) {
                ImGui::SetCursorPosY( y );
            }
            ImGui::TextColored( c_green, "%s", msg.c_str() );
            msg.clear();
            input_context ctxt( menu->input_category, keyboard_mode::keycode );
            ImGui::Text( _( "[%s] find, [%s] quit, [t] toggle base trait" ),
                         ctxt.get_desc( "FILTER" ).c_str(), ctxt.get_desc( "QUIT" ).c_str() );

            if( only_active ) {
                ImGui::TextColored( c_green, "%s", _( "[a] show active traits (active)" ) );
            } else {
                ImGui::TextColored( c_white, "%s", _( "[a] show active traits" ) );
            }
        }

        ~wish_mutate_callback() override = default;
};

void debug_menu::wishmutate( Character *you )
{
    uilist wmenu;
    int c = 0;

    for( const mutation_branch &traits_iter : mutation_branch::get_all() ) {
        wmenu.addentry( -1, true, -2, traits_iter.name() );
        wmenu.entries[ c ].extratxt.left = 1;
        wmenu.entries[ c ].extratxt.txt.clear();
        wmenu.entries[ c ].extratxt.color = c_light_green;
        if( you->has_trait( traits_iter.id ) ) {
            wmenu.entries[ c ].txt = string_format( _( "%s (active)" ), traits_iter.name() );
            wmenu.entries[ c ].text_color = c_green;
            if( you->has_base_trait( traits_iter.id ) ) {
                wmenu.entries[ c ].extratxt.txt = "T";
            }
        } else {
            wmenu.entries[ c ].txt = traits_iter.name();
        }
        c++;
    }
    wmenu.desired_bounds = { -1.0, -1.0, 1.0, 1.0 };
    wmenu.selected = uistate.wishmutate_selected;
    wish_mutate_callback cb;
    cb.you = you;
    wmenu.callback = &cb;
    do {
        wmenu.query();
        if( wmenu.ret >= 0 ) {
            int rc = 0;
            const trait_id mstr = cb.vTraits[ wmenu.ret ];
            const mutation_branch &mdata = mstr.obj();
            const bool threshold = mdata.threshold;
            const bool profession = mdata.profession;
            // Manual override for the threshold-gaining
            if( threshold || profession ) {
                if( you->has_permanent_trait( mstr ) ) {
                    do {
                        you->remove_mutation( mstr );
                        rc++;
                    } while( you->has_permanent_trait( mstr ) && rc < 10 );
                } else {
                    do {
                        you->set_mutation( mstr );
                        rc++;
                    } while( !you->has_permanent_trait( mstr ) && rc < 10 );
                }
            } else if( you->has_permanent_trait( mstr ) ) {
                do {
                    you->remove_mutation( mstr );
                    rc++;
                } while( you->has_permanent_trait( mstr ) && rc < 10 );
            } else {
                do {
                    you->mutate_towards( mstr );
                    rc++;
                } while( !you->has_permanent_trait( mstr ) && rc < 10 );
            }
            cb.msg = string_format( _( "%s Mutation changes: %d" ), mstr.c_str(), rc );
            uistate.wishmutate_selected = wmenu.selected;
            if( rc != 0 ) {
                for( size_t i = 0; i < cb.vTraits.size(); i++ ) {
                    uilist_entry &entry = wmenu.entries[ i ];
                    entry.extratxt.txt.clear();
                    if( you->has_trait( cb.vTraits[ i ] ) ) {
                        entry.txt = string_format( _( "%s (active)" ), cb.vTraits[ i ]->name() );
                        entry.enabled = true;
                        entry.text_color = c_green;
                        cb.pTraits[ cb.vTraits[ i ] ] = true;
                        if( you->has_base_trait( cb.vTraits[ i ] ) ) {
                            entry.extratxt.txt = "T";
                        }
                    } else {
                        entry.txt = cb.vTraits[ i ]->name();
                        entry.enabled = entry.enabled ? true : !cb.only_active;
                        entry.text_color = wmenu.text_color;
                        cb.pTraits[ cb.vTraits[ i ] ] = false;
                    }
                }
            }
            wmenu.filterlist();
        }
    } while( wmenu.ret >= 0 );
}

void debug_menu::wishbionics( Character *you )
{
    std::vector<const itype *> cbm_items = item_controller->find( []( const itype & itm ) -> bool {
        return itm.can_use( "install_bionic" );
    } );
    std::sort( cbm_items.begin(), cbm_items.end(), []( const itype * a, const itype * b ) {
        return localized_compare( a->nname( 1 ), b->nname( 1 ) );
    } );

    while( true ) {
        units::energy power_level = you->get_power_level();
        units::energy power_max = you->get_max_power_level();
        size_t num_installed = you->get_bionics().size();

        bool can_uninstall = num_installed > 0;
        bool can_uninstall_all = can_uninstall || power_max > 0_J;

        uilist smenu;
        smenu.text += string_format(
                          _( "Current power level: %s\nMax power: %s\nBionics installed: %d" ),
                          units::display( power_level ),
                          units::display( power_max ),
                          num_installed
                      );
        smenu.addentry( 0, true, 'i', _( "Install from CBM…" ) );
        smenu.addentry( 1, can_uninstall, 'u', _( "Uninstall…" ) );
        smenu.addentry( 2, can_uninstall_all, 'U', _( "Uninstall all" ) );
        smenu.addentry( 3, true, 'c', _( "Edit power capacity (kJ)" ) );
        smenu.addentry( 4, true, 'C', _( "Edit power capacity (J)" ) );
        smenu.addentry( 5, true, 'p', _( "Edit power level (kJ)" ) );
        smenu.addentry( 6, true, 'P', _( "Edit power level (J)" ) );
        smenu.query();
        switch( smenu.ret ) {
            case 0: {
                uilist scbms;
                for( size_t i = 0; i < cbm_items.size(); i++ ) {
                    bool enabled = !you->has_bionic( cbm_items[i]->bionic->id );
                    scbms.addentry( i, enabled, MENU_AUTOASSIGN, "%s", cbm_items[i]->nname( 1 ) );
                }
                scbms.query();
                if( scbms.ret >= 0 ) {
                    const itype &cbm = *cbm_items[scbms.ret];
                    const bionic_id &bio = cbm.bionic->id;
                    constexpr int difficulty = 0;
                    constexpr int success = 1;
                    constexpr int level = 99;

                    bionic_uid upbio_uid = 0;
                    if( std::optional<bionic *> upbio = you->find_bionic_by_type( bio->upgraded_bionic ) ) {
                        upbio_uid = ( *upbio )->get_uid();
                    }

                    you->perform_install( bio, upbio_uid, difficulty, success, level, "NOT_MED",
                                          bio->canceled_mutations,
                                          you->pos_bub() );
                }
                break;
            }
            case 1: {
                const bionic_collection &installed_bionics = *you->my_bionics;
                std::vector<std::string> bionic_names;
                std::vector<const bionic *> bionics;
                for( const bionic &bio : installed_bionics ) {
                    if( item::type_is_defined( bio.info().itype() ) ) {
                        bionic_names.emplace_back( bio.info().name.translated() );
                        bionics.push_back( &bio );
                    }
                }
                int bionic_index = uilist( _( "Choose bionic to uninstall" ), bionic_names );
                if( bionic_index < 0 ) {
                    return;
                }

                you->remove_bionic( *bionics[bionic_index] );
                break;
            }
            case 2: {
                you->clear_bionics();
                you->set_power_level( units::from_kilojoule( 0 ) );
                you->set_max_power_level( units::from_kilojoule( 0 ) );
                break;
            }
            case 3: {
                int new_value = 0;
                if( query_int( new_value, false, _( "Set the value to (in kJ)?  Currently: %s" ),
                               units::display( power_max ) ) ) {
                    you->set_max_power_level( units::from_kilojoule( static_cast<std::int64_t>( new_value ) ) );
                    you->set_power_level( you->get_power_level() );
                }
                break;
            }
            case 4: {
                int new_value = 0;
                if( query_int( new_value, false, _( "Set the value to (in J)?  Currently: %s" ),
                               units::display( power_max ) ) ) {
                    you->set_max_power_level( units::from_joule( static_cast<std::int64_t>( new_value ) ) );
                    you->set_power_level( you->get_power_level() );
                }
                break;
            }
            case 5: {
                int new_value = 0;
                if( query_int( new_value, false, _( "Set the value to (in kJ)?  Currently: %s" ),
                               units::display( power_level ) ) ) {
                    you->set_power_level( units::from_kilojoule( static_cast<std::int64_t>( new_value ) ) );
                }
                break;
            }
            case 6: {
                int new_value = 0;
                if( query_int( new_value, false, _( "Set the value to (in J)?  Currently: %s" ),
                               units::display( power_level ) ) ) {
                    you->set_power_level( units::from_joule( static_cast<std::int64_t>( new_value ) ) );
                }
                break;
            }
            default: {
                return;
            }
        }
    }
}

void debug_menu::wisheffect( Creature &p )
{
    static bodypart_str_id effectbp = bodypart_str_id::NULL_ID();
    std::vector<effect> effects;
    uilist efmenu;
    efmenu.desc_enabled = true;
    const int offset = 2;
    bool only_active = false;

    const size_t effect_size = get_effect_types().size();
    effects.reserve( effect_size );

    auto effect_description = []( const effect & eff ) -> std::string {
        const effect_type &efft = *eff.get_effect_type();
        std::ostringstream descstr;

        if( eff.get_effect_type()->use_name_ints() )
        {
            descstr << eff.disp_name() << '\n';
        }

        descstr << _( "Intensity threshold: " );
        descstr <<  colorize( std::to_string( to_seconds<int>( efft.intensity_duration() ) ),
                              c_yellow );
        descstr << "s | ";

        descstr << _( "Max: " );
        int max_duration = to_seconds<int>( eff.get_max_duration() );
        descstr << colorize( std::to_string( max_duration ), c_yellow );
        descstr << "s\n";

        if( eff.get_effect_type()->use_desc_ints( false ) )
        {
            descstr << eff.disp_desc( false ) << '\n';
        }
        descstr << eff.disp_mod_source_info() << '\n';

        return descstr.str();
    };

    auto rebuild_menu = [&]( const bodypart_str_id & bp ) -> void {
        effects.clear();
        efmenu.entries.clear();
        efmenu.title = string_format( _( "Debug Effects Menu: %s" ), bp->id.str() );
        efmenu.addentry( 0, true, 'a', _( "Show only active" ) );
        efmenu.addentry( 1, true, 'b', _( "Change body part" ) );
        only_active = false;

        for( const std::pair<const efftype_id, effect_type> &eff : get_effect_types() )
        {
            const effect &plyeff = p.get_effect( eff.first, bp );
            if( plyeff.is_null() ) {
                effects.emplace_back( &*eff.first );
            } else {
                effects.emplace_back( plyeff );
            }
        }

        std::sort( effects.begin(), effects.end(), []( const effect & effA, const effect & effB )
        {
            return localized_compare( effA.get_id().str(), effB.get_id().str() );
        } );

        for( size_t i = 0; i < effect_size; ++i )
        {
            const effect &eff = effects[i];
            uilist_entry entry{static_cast<int>( i + offset ), true, -2, eff.get_id().str()};

            int duration = to_seconds<int>( eff.get_duration() );
            if( duration ) {
                entry.ctxt = colorize( std::to_string( duration ), c_white );
                if( eff.is_permanent() ) {
                    entry.ctxt += colorize( " PERMANENT", c_white );
                }
            }

            entry.desc = effect_description( eff );
            efmenu.entries.emplace_back( entry );
        }
    };

    rebuild_menu( effectbp );

    do {
        efmenu.query();
        if( efmenu.ret == 0 ) {
            only_active = !only_active;
            for( uilist_entry &entry : efmenu.entries ) {
                if( only_active ) {
                    if( entry.retval >= offset ) {
                        const int duration = to_seconds<int>( effects[entry.retval - offset].get_duration() );
                        entry.enabled = duration > 0 || entry.retval < offset;
                    }
                } else {
                    entry.enabled = true;
                }
            }
            continue;
        } else if( efmenu.ret == 1 ) {
            uilist bpmenu;
            bpmenu.title = _( "Choose bodypart" );
            bpmenu.addentry( 0, true, 'a', bodypart_str_id::NULL_ID()->id.str() );

            const std::vector<bodypart_id> &bodyparts = p.get_all_body_parts();
            const size_t bodyparts_size = bodyparts.size();
            for( int i = 0; i < static_cast<int>( bodyparts_size ); ++i ) {
                bpmenu.addentry( i + 1, true, -1, bodyparts[i]->id.str() );
            }

            bpmenu.query();
            if( bpmenu.ret == 0 ) {
                effectbp = bodypart_str_id::NULL_ID();
            } else if( bpmenu.ret > 0 ) {
                effectbp = bodyparts[bpmenu.ret - 1]->id;
            }
            rebuild_menu( effectbp );
        } else if( efmenu.ret > offset - 1 ) {
            uilist_entry &entry = efmenu.entries[efmenu.ret];
            effect &eff = effects[efmenu.ret - offset];

            int duration = to_seconds<int>( eff.get_duration() );
            query_int( duration, false, _( "Set duration (current %1$d): " ), duration );
            if( duration < 0 ) {
                continue;
            }

            bool permanent = false;
            if( duration ) {
                permanent = query_yn( _( "Permanent?" ) );
            }

            p.remove_effect( eff.get_id(), effectbp );
            bool invalid_effect = false;
            if( duration > 0 ) {
                p.add_effect( eff.get_id(), time_duration::from_seconds( duration ), effectbp, permanent );
                // Some effects like bandages on a limb like foot_l
                // cause a segmentation fault, check if it was applied first
                const effect &new_effect = p.get_effect( eff.get_id(), effectbp );
                if( !new_effect.is_null() ) {
                    eff = new_effect;
                } else {
                    invalid_effect = true;
                }
            } else {
                eff.set_duration( 0_seconds );
                if( only_active ) {
                    entry.enabled = false;
                }
            }

            entry.ctxt.clear();
            entry.desc.clear();

            if( invalid_effect ) {
                entry.ctxt += colorize( _( "INVALID ON THIS LIMB" ), c_red );
                entry.desc += colorize( _( "This effect can not be applied on this limb" ), c_red );
                entry.desc += '\n';
            } else {
                int cur_duration = to_seconds<int>( p.get_effect_dur( eff.get_id(), effectbp ) );
                if( cur_duration ) {
                    entry.ctxt = colorize( std::to_string( cur_duration ), c_yellow );
                    if( eff.is_permanent() ) {
                        entry.ctxt += colorize( _( " PERMANENT" ), c_yellow );
                    }
                }
            }

            entry.desc += effect_description( eff );

        }
    } while( efmenu.ret != UILIST_CANCEL );
}

class wish_monster_callback: public uilist_callback
{
    public:
        // last menu entry
        int lastent;
        // feedback message
        std::string msg;
        // spawn friendly critter?
        bool friendly;
        bool hallucination;
        // Number of monsters to spawn.
        int group;
        // scrap critter for monster::print_info
        monster tmp;
        const std::vector<const mtype *> &mtypes;

        explicit wish_monster_callback( const std::vector<const mtype *> &mtypes )
            : mtypes( mtypes ) {
            friendly = false;
            hallucination = false;
            group = 0;
            lastent = -2;
        }

        bool key( const input_context &, const input_event &event, int /*entnum*/,
                  uilist * /*menu*/ ) override {
            if( event.get_first_input() == 'f' ) {
                friendly = !friendly;
                // Force tmp monster regen
                lastent = -2;
                // Tell menu we handled keypress
                return true;
            } else if( event.get_first_input() == 'i' ) {
                group++;
                return true;
            } else if( event.get_first_input() == 'h' ) {
                hallucination = !hallucination;
                return true;
            } else if( event.get_first_input() == 'd' && group != 0 ) {
                group--;
                return true;
            }
            return false;
        }

        float desired_extra_space_right( ) override {
            return 30 * ImGui::CalcTextSize( "X" ).x;
        }

        void refresh( uilist *menu ) override {
            ImVec2 info_size = ImGui::GetContentRegionAvail( );
            info_size.x = desired_extra_space_right( );
            ImGui::TableSetColumnIndex( 2 );
            if( ImGui::BeginChild( "monster info", info_size ) ) {
                const int entnum = menu->previewing;
                const bool valid_entnum = entnum >= 0 && static_cast<size_t>( entnum ) < mtypes.size();
                if( entnum != lastent ) {
                    lastent = entnum;
                    if( valid_entnum ) {
                        tmp = monster( mtypes[ entnum ]->id );
                        if( friendly ) {
                            tmp.friendly = -1;
                        }
                    } else {
                        tmp = monster();
                    }
                }

                if( valid_entnum ) {
                    std::string header = string_format( "#%d: %s (%d)%s", entnum, tmp.type->id.c_str(), group,
                                                        hallucination ? _( " (hallucination)" ) : "" );
                    ImGui::SetCursorPosX( ( ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize(
                                                header.c_str() ).x ) * 0.5 );
                    ImGui::TextColored( c_cyan, "%s", header.c_str() );

                    tmp.print_info_imgui();
                }

                float y = ImGui::GetContentRegionMax().y - 3 * ImGui::GetTextLineHeightWithSpacing();
                if( ImGui::GetCursorPosY() < y ) {
                    ImGui::SetCursorPosY( y );
                }
                ImGui::TextColored( c_green, "%s", msg.c_str() );
                msg.clear();
                input_context ctxt( menu->input_category, keyboard_mode::keycode );
                ImGui::Text(
                    _( "[%s] find, [f]riendly, [h]allucination, [i]ncrease group, [d]ecrease group, [%s] quit" ),
                    ctxt.get_desc( "FILTER" ).c_str(), ctxt.get_desc( "QUIT" ).c_str() );
            }
            ImGui::EndChild();
        }

        ~wish_monster_callback() override = default;
};

static void setup_wishmonster( uilist &pick_a_monster, std::vector<const mtype *> &mtypes )
{
    pick_a_monster.desired_bounds = { -1.0, -1.0, 1.0, 1.0 };
    pick_a_monster.selected = uistate.wishmonster_selected;
    int i = 0;
    for( const mtype &montype : MonsterGenerator::generator().get_all_mtypes() ) {
        pick_a_monster.addentry( i, true, 0, montype.nname() );
        pick_a_monster.entries[i].extratxt.txt = montype.sym;
        pick_a_monster.entries[i].extratxt.color = montype.color;
        pick_a_monster.entries[i].extratxt.left = 1;
        ++i;
        mtypes.push_back( &montype );
    }
}

void debug_menu::wishmonstergroup( tripoint_abs_omt &loc )
{
    bool population_group = query_yn(
                                _( "Is this a population-based horde, based on an existing monster group?  Select \"No\" to manually assign monsters." ) );
    mongroup new_group;
    // Just in case, we manually set some values
    new_group.abs_pos = project_to<coords::sm>( loc );
    new_group.target = new_group.abs_pos.xy();
    new_group.population = 1; // overwritten if population_group
    new_group.horde = true;
    new_group.type = GROUP_ZOMBIE; // overwritten if population_group
    if( population_group ) {
        uilist type_selection;
        std::vector<mongroup_id> possible_groups;
        int i = 0;
        for( auto &group_pair : MonsterGroupManager::Get_all_Groups() ) {
            possible_groups.emplace_back( group_pair.first );
            type_selection.addentry( i, true, -1, group_pair.first.c_str() );
            i++;
        }
        type_selection.query();
        int &selected = type_selection.ret;
        if( selected < 0 || static_cast<size_t>( selected ) >= possible_groups.size() ) {
            return;
        }
        const mongroup_id selected_group( possible_groups[selected] );
        new_group.type = selected_group;
        int new_value = new_group.population; // default value if query declined
        query_int( new_value, false, _( "Set population to what value?  Currently %d" ),
                   new_group.population );
        new_group.population = new_value;
        overmap &there = overmap_buffer.get( project_to<coords::om>( loc ).xy() );
        there.debug_force_add_group( new_group );
        return; // Don't go to adding individual monsters, they'll override the values we just set
    }


    wishmonstergroup_mon_selection( new_group );
    overmap &there = overmap_buffer.get( project_to<coords::om>( loc ).xy() );
    there.debug_force_add_group( new_group );
}

void debug_menu::wishmonstergroup_mon_selection( mongroup &group )
{

    std::vector<const mtype *> mtypes;
    uilist new_mon_selection;
    setup_wishmonster( new_mon_selection, mtypes );
    wish_monster_callback cb( mtypes );
    new_mon_selection.callback = &cb;
    new_mon_selection.query();
    int &selected = new_mon_selection.ret;
    if( selected < 0 || static_cast<size_t>( selected ) >= mtypes.size() ) {
        return;
    }
    const mtype_id &mon_type = mtypes[ selected ]->id;
    for( int i = 0; i < cb.group; i++ ) {
        monster new_mon( mon_type );
        if( cb.friendly ) {
            new_mon.friendly = -1;
            new_mon.add_effect( effect_pet, 1_turns, true );
        }
        if( cb.hallucination ) {
            new_mon.hallucination = true;
        }
        group.monsters.emplace_back( new_mon );
    }
}

void debug_menu::wishmonster( const std::optional<tripoint_bub_ms> &p )
{
    std::vector<const mtype *> mtypes;

    uilist wmenu;
    setup_wishmonster( wmenu, mtypes );
    wish_monster_callback cb( mtypes );
    wmenu.callback = &cb;

    do {
        wmenu.query();
        if( wmenu.ret >= 0 ) {
            const mtype_id &mon_type = mtypes[ wmenu.ret ]->id;
            if( std::optional<tripoint_bub_ms> spawn = p.has_value() ? p : g->look_around() ) {
                int num_spawned = 0;
                for( const tripoint_bub_ms &destination : closest_points_first( *spawn, cb.group ) ) {
                    monster *const mon = g->place_critter_at( mon_type, destination );
                    if( !mon ) {
                        continue;
                    }
                    if( cb.friendly ) {
                        mon->friendly = -1;
                        mon->add_effect( effect_pet, 1_turns, true );
                    }
                    if( cb.hallucination ) {
                        mon->hallucination = true;
                    }
                    ++num_spawned;
                }
                input_context ctxt( wmenu.input_category, keyboard_mode::keycode );
                cb.msg = string_format( _( "Spawned %d monsters, choose another or [%s] to quit." ),
                                        num_spawned, ctxt.get_desc( "QUIT" ) );
                if( num_spawned == 0 ) {
                    cb.msg += _( "\nTarget location is not suitable for placing this kind of monster.  Choose a different target or [i]ncrease the groups size." );
                }
                uistate.wishmonster_selected = wmenu.selected;
            }
        }
    } while( wmenu.ret >= 0 );
}

static item wishitem_produce( const itype &type, std::string &flags, bool incontainer )
{
    item granted( &type, calendar::turn );

    granted.unset_flags();
    for( const auto &tag : debug_menu::string_to_iterable<std::vector<std::string>>( flags, " " ) ) {
        const flag_id flag( tag );
        if( flag.is_valid() ) {
            granted.set_flag( flag_id( tag ) );
        }
    }

    if( incontainer ) {
        granted = granted.in_its_container();
    }
    // If the item has an ammunition, this loads it to capacity, including magazines.
    if( !granted.ammo_default().is_null() ) {
        granted.ammo_set( granted.ammo_default(), -1 );
    }

    return granted;
}

class wish_item_callback: public uilist_callback
{
    public:
        int examine_pos;
        bool incontainer;
        bool spawn_everything;
        bool renew_snippet;
        std::string msg;
        std::string flags;
        std::string itype_flags;
        std::pair<int, std::string> chosen_snippet_id;
        const std::vector<const itype *> &standard_itype_ids;
        const std::vector<const itype_variant_data *> &itype_variants;
        std::string &last_snippet_id;

        int entnum = -1;
        std::string header;
        std::vector<iteminfo> info;
        item tmp;

        explicit wish_item_callback( const std::vector<const itype *> &ids,
                                     const std::vector<const itype_variant_data *> &variants, std::string &snippet_ids ) :
            incontainer( false ), spawn_everything( false ),
            standard_itype_ids( ids ), itype_variants( variants ),
            last_snippet_id( snippet_ids ) {
        }

        void select( uilist *menu ) override {
            if( menu->selected < 0 ) {
                return;
            }
            examine_pos = 0;
            chosen_snippet_id = { -1, "" };
            renew_snippet = true;
            const itype &selected_itype = *standard_itype_ids[menu->selected];
            // Make liquids "contained" by default (toggled with CONTAINER action)
            incontainer = selected_itype.phase == phase_id::LIQUID;
            // Clear instance flags when switching items
            flags.clear();
            // Grab default flags for the itype (added with the FLAG action)
            itype_flags = debug_menu::iterable_to_string( selected_itype.get_flags(), " ",
            []( const flag_id & f ) {
                return f.str();
            } );
        }

        bool key( const input_context &ctxt, const input_event &event, int /*entnum*/,
                  uilist *menu ) override {

            const std::string &action = ctxt.input_to_action( event );
            const int cur_key = event.get_first_input();
            if( action == "CONTAINER" ) {
                incontainer = !incontainer;
                return true;
            }
            if( action == "FLAG" ) {
                std::string edit_flags;
                if( flags.empty() ) {
                    // If this is the first time using the FLAG action on this item, start with itype flags
                    edit_flags = itype_flags;
                } else {
                    // Otherwise, edit the existing list of user-defined instance flags
                    edit_flags = flags;
                }
                string_input_popup popup;
                popup
                .title( _( "Flags:" ) )
                .description( _( "UPPERCASE, no quotes, separate with spaces" ) )
                .max_length( 100 )
                .text( edit_flags )
                .query();
                // Save instance flags on this item (will be reset when selecting another item)
                if( popup.confirmed() ) {
                    flags = popup.text();
                    return true;
                }
            }
            if( action == "EVERYTHING" ) {
                spawn_everything = !spawn_everything;
                return true;
            }
            if( action == "SNIPPET" ) {
                if( menu->selected <= -1 ) {
                    return true;
                }
                const int entnum = menu->selected;
                const itype &selected_itype = *standard_itype_ids[entnum];
                if( !selected_itype.snippet_category.empty() ) {
                    const std::string cat = selected_itype.snippet_category;
                    if( SNIPPET.has_category( cat ) ) {
                        std::vector<std::pair<snippet_id, std::string>> snippes = SNIPPET.get_snippets_by_category( cat,
                                true );
                        if( !snippes.empty() ) {
                            uilist snipp_query;
                            snipp_query.text = _( "Choose snippet type." );
                            snipp_query.desc_enabled = true;
                            int cnt = 0;
                            for( const std::pair<snippet_id, std::string> &elem : snippes ) {
                                std::string desc = elem.second;
                                snipp_query.addentry_desc( cnt, true, -1, elem.first.str(), desc );
                                cnt ++;
                            }
                            snipp_query.query();
                            switch( snipp_query.ret ) {
                                case UILIST_CANCEL:
                                    break;
                                default:
                                    chosen_snippet_id = { entnum, snippes[snipp_query.ret].first.str() };
                                    break;
                            }
                        }
                    }
                }
                return true;
            }
            if( action == "SCROLL_DESC_UP" ) {
                examine_pos = std::max( examine_pos - 1, 0 );
            }
            if( action == "SCROLL_DESC_DOWN" ) {
                examine_pos += 1;
            }
            if( cur_key == KEY_LEFT || cur_key == KEY_RIGHT ) {
                // For Renew snippet_id.
                renew_snippet = true;
                return true;
            }
            return false;
        }

        float desired_extra_space_right( ) override {
            return std::min( ImGui::GetMainViewport()->Size.x / 2.0f,
                             std::max( TERMX / 2, TERMX - 50 ) * ImGui::CalcTextSize( "X" ).x );
        }

        void refresh( uilist *menu ) override {
            const int entnum = menu->previewing;
            if( entnum >= 0 && static_cast<size_t>( entnum ) < standard_itype_ids.size() ) {
                tmp = wishitem_produce( *standard_itype_ids[entnum], flags, false );

                const itype_variant_data *variant = itype_variants[entnum];
                if( variant != nullptr && tmp.has_itype_variant( false ) ) {
                    // Set the variant type as shown in the selected list item.
                    std::string variant_id = variant->id;
                    tmp.set_itype_variant( variant_id );
                }

                if( !tmp.type->snippet_category.empty() ) {
                    if( renew_snippet ) {
                        last_snippet_id = tmp.snip_id.str();
                        renew_snippet = false;
                    } else if( chosen_snippet_id.first == entnum && !chosen_snippet_id.second.empty() ) {
                        std::string snip = chosen_snippet_id.second;
                        if( snippet_id( snip ).is_valid() || snippet_id( snip ) == snippet_id::NULL_ID() ) {
                            tmp.snip_id = snippet_id( snip );
                            last_snippet_id = snip;
                        }
                    } else {
                        tmp.snip_id = snippet_id( last_snippet_id );
                    }
                }

                header = string_format( "#%d: %s%s%s", entnum,
                                        standard_itype_ids[entnum]->get_id().c_str(),
                                        incontainer ? _( " (contained)" ) : "",
                                        flags.empty() ? "" : _( " (flagged)" ) );
                info = tmp.get_info( true );
            }

            ImVec2 info_size = ImGui::GetContentRegionAvail();
            info_size.x = desired_extra_space_right( );
            info_size.y -= ( 3.0 * ImGui::GetTextLineHeightWithSpacing() ) - ImGui::GetFrameHeightWithSpacing();

            ImGui::TableSetColumnIndex( 2 );
            if( ImGui::BeginChild( "wish item", info_size ) ) {
                ImGui::SetCursorPosX( ( info_size.x - ImGui::CalcTextSize( header.c_str() ).x ) * 0.5 );
                ImGui::TextColored( c_cyan, "%s", header.c_str() );
                display_item_info( info, {} );
            }
            ImGui::EndChild();

            ImGui::TextColored( c_green, "%s", msg.c_str() );
            input_context ctxt( menu->input_category, keyboard_mode::keycode );
            ImGui::Text( _( "[%s] find, [%s] container, [%s] flag, [%s] everything, [%s] snippet, [%s] quit" ),
                         ctxt.get_desc( "FILTER" ).c_str(),
                         ctxt.get_desc( "CONTAINER" ).c_str(),
                         ctxt.get_desc( "FLAG" ).c_str(),
                         ctxt.get_desc( "EVERYTHING" ).c_str(),
                         ctxt.get_desc( "SNIPPET" ).c_str(),
                         ctxt.get_desc( "QUIT" ).c_str() );
        }
};

void debug_menu::wishitem( Character *you )
{
    wishitem( you, tripoint_bub_ms( -1, -1, -1 ) );
}

void debug_menu::wishitem( Character *you, const tripoint_bub_ms &pos )
{
    if( you == nullptr && pos.x() <= 0 ) {
        debugmsg( "game::wishitem(): invalid parameters" );
        return;
    }
    std::vector<std::tuple<std::string, const itype *, const itype_variant_data *>> opts;
    for( const itype *i : item_controller->all() ) {
        item option( i, calendar::turn_zero );

        if( i->variant_kind == itype_variant_kind::gun || i->variant_kind == itype_variant_kind::generic ) {
            for( const itype_variant_data &variant : i->variants ) {
                const std::string gun_variant_name = variant.alt_name.translated();
                const itype_variant_data *ivd = &variant;
                opts.emplace_back( gun_variant_name, i, ivd );
            }
        }
        option.clear_itype_variant();
        opts.emplace_back( option.tname( 1, false ), i, nullptr );
    }
    std::sort( opts.begin(), opts.end(), localized_compare );
    std::vector<const itype *> itypes;
    std::vector<const itype_variant_data *> ivariants;
    std::transform( opts.begin(), opts.end(), std::back_inserter( itypes ),
    []( const auto & pair ) {
        return std::get<1>( pair );
    } );

    std::transform( opts.begin(), opts.end(), std::back_inserter( ivariants ),
    []( const auto & pair ) {
        return std::get<2>( pair );
    } );

    int prev_amount = 1;
    int amount = 1;
    std::string snipped_id_str;
    uilist wmenu;
    wmenu.input_category = "WISH_ITEM";
    wmenu.additional_actions = {
        { "CONTAINER", translation() },
        { "FLAG", translation() },
        { "EVERYTHING", translation() },
        { "SNIPPET", translation() },
        { "SCROLL_DESC_UP", translation() },
        { "SCROLL_DESC_DOWN", translation() },
    };
    wmenu.desired_bounds = { -0.9, -0.9, 0.9, 0.9 };
    wmenu.selected = uistate.wishitem_selected;
    wish_item_callback cb( itypes, ivariants, snipped_id_str );
    wmenu.callback = &cb;

    for( size_t i = 0; i < opts.size(); i++ ) {
        item ity( std::get<1>( opts[i] ), calendar::turn_zero );
        std::string i_name = std::get<0>( opts[i] );
        if( std::get<2>( opts[i] ) != nullptr ) {
            i_name += "<color_dark_gray>(V)</color>";
        }
        if( !std::get<1>( opts[i] )->snippet_category.empty() ) {
            i_name += "<color_yellow>(S)</color>";
        }

        wmenu.addentry( i, true, 0, i_name );
        mvwzstr &entry_extra_text = wmenu.entries[i].extratxt;
        entry_extra_text.txt = ity.symbol();
        entry_extra_text.color = ity.color();
        entry_extra_text.left = 1;
    }
    do {
        wmenu.query();
        if( cb.spawn_everything ) {
            wmenu.ret = opts.size() - 1;
        }
        bool did_amount_prompt = false;
        while( wmenu.ret >= 0 ) {
            item granted = wishitem_produce( *std::get<1>( opts[wmenu.ret] ), cb.flags, cb.incontainer );
            const itype_variant_data *variant = std::get<2>( opts[wmenu.ret] );
            if( variant != nullptr && granted.has_itype_variant( false ) ) {
                std::string variant_id = variant->id;
                granted.set_itype_variant( variant_id );
            }
            if( !granted.type->snippet_category.empty() && ( snippet_id( snipped_id_str ).is_valid() ||
                    snippet_id( snipped_id_str ) == snippet_id::NULL_ID() ) ) {
                granted.snip_id = snippet_id( snipped_id_str );
            }

            prev_amount = amount;
            bool canceled = false;
            if( you != nullptr && !did_amount_prompt ) {
                string_input_popup popup;
                popup
                .title( _( "How many?" ) )
                .width( 20 )
                .description( granted.tname() )
                .edit( amount );
                canceled = popup.canceled();
            }
            if( !canceled ) {
                did_amount_prompt = true;
                if( you != nullptr ) {
                    if( amount > 0 ) {
                        int stashable_copy_num = amount;
                        you->i_add( granted, stashable_copy_num, true, nullptr, nullptr, true, false );
                    }
                    you->invalidate_crafting_inventory();
                } else if( pos.x() >= 0 && pos.y() >= 0 ) {
                    get_map().add_item_or_charges( pos, granted );
                    wmenu.ret = -1;
                }
                if( amount > 0 ) {
                    input_context ctxt( wmenu.input_category, keyboard_mode::keycode );
                    cb.msg = string_format( _( "Wish granted.  Wish for more or hit [%s] to quit." ),
                                            ctxt.get_desc( "QUIT" ) );
                }
            }
            uistate.wishitem_selected = wmenu.selected;
            if( canceled || amount <= 0 ) {
                amount = prev_amount;
            }
            if( cb.spawn_everything ) {
                wmenu.ret--;
            } else {
                break;
            }
        }
    } while( wmenu.ret >= 0 );
}

void debug_menu::wishitemgroup( bool test )
{
    std::vector<item_group_id> groups = item_controller->get_all_group_names();
    uilist menu;
    for( size_t i = 0; i < groups.size(); i++ ) {
        menu.entries.emplace_back( static_cast<int>( i ), true, -2, groups[i].str() );
    }
    while( true ) {
        menu.query();
        const int index = menu.ret;
        if( index >= static_cast<int>( groups.size() ) || index < 0 ) {
            break;
        }
        size_t amount = 0;
        number_input_popup<int> popup( 0, test ? 100 : 1, _( "Spawn group how many times?" ) );
        const int &ret = popup.query();
        if( popup.cancelled() || ret < 1 ) {
            return;
        }
        amount = static_cast<size_t>( ret );
        if( !test ) {
            const std::optional<tripoint_bub_ms> p = g->look_around();
            if( !p ) {
                return;
            }
            for( size_t a = 0; a < amount; a++ ) {
                for( const item &it : item_group::items_from( groups[index], calendar::turn ) ) {
                    get_map().add_item_or_charges( *p, it );
                }
            }
        } else {
            std::map<std::string, int> itemnames;
            for( size_t a = 0; a < amount; a++ ) {
                for( const item &it : item_group::items_from( groups[index], calendar::turn ) ) {
                    itemnames[it.display_name()]++;
                }
            }
            // Flip the map keys/values and use reverse sorting so common items are first
            std::multimap <int, std::string, std::greater<>> itemnames_by_popularity;
            for( const auto &e : itemnames ) {
                itemnames_by_popularity.insert( std::pair<int, std::string>( e.second, e.first ) );
            }
            uilist results_menu;
            results_menu.text = string_format( _( "Potential result of spawning %s %d %s:" ),
                                               groups[index].c_str(), amount, amount == 1 ? _( "time" ) : _( "times" ) );
            for( const auto &e : itemnames_by_popularity ) {
                results_menu.entries.emplace_back( static_cast<int>( results_menu.entries.size() ), true, -2,
                                                   string_format( _( "%d x %s" ), e.first, e.second ) );
            }
            results_menu.query();
        }
    }
}

void debug_menu::wishskill( Character *you, bool change_theory )
{
    const int skoffset = 1;
    uilist skmenu;
    skmenu.title = change_theory ?
                   _( "Select a skill to modify its theory level" ) : _( "Select a skill to modify" );
    skmenu.allow_anykey = true;
    skmenu.additional_actions = {
        { "LEFT", to_translation( "Decrease skill" ) },
        { "RIGHT", to_translation( "Increase skill" ) }
    };
    skmenu.addentry( 0, true, '1', _( "Modify all skills…" ) );

    auto sorted_skills = Skill::get_skills_sorted_by( []( const Skill & a, const Skill & b ) {
        return localized_compare( a.name(), b.name() );
    } );

    auto get_level = [&change_theory, &you]( const Skill & skill ) -> int {
        return change_theory ?
        you->get_knowledge_level( skill.ident() ) : you->get_skill_level( skill.ident() );
    };

    std::vector<int> origskills;
    origskills.reserve( sorted_skills.size() );

    for( const Skill *s : sorted_skills ) {
        const int level = get_level( *s );
        skmenu.addentry( origskills.size() + skoffset, true, -2, _( "@ %d: %s  " ), level,
                         s->name() );
        origskills.push_back( level );
    }

    shared_ptr_fast<uilist_impl> skmenu_ui = skmenu.create_or_get_ui();

    do {
        skmenu.query();
        int skill_id = -1;
        int skset = -1;
        const int sksel = skmenu.selected - skoffset;
        if( skmenu.ret == UILIST_UNBOUND && ( skmenu.ret_act == "LEFT" ||
                                              skmenu.ret_act == "RIGHT" ) ) {
            if( sksel >= 0 && sksel < static_cast<int>( sorted_skills.size() ) ) {
                skill_id = sksel;
                skset = get_level( *sorted_skills[skill_id] ) +
                        ( skmenu.ret_act == "LEFT" ? -1 : 1 );
            }
        } else if( skmenu.ret >= 0 && sksel >= 0 &&
                   sksel < static_cast<int>( sorted_skills.size() ) ) {
            skill_id = sksel;
            const Skill &skill = *sorted_skills[skill_id];
            uilist sksetmenu;
            // auto skmenu_bounds = skmenu.get_bounds();
            // sksetmenu.desired_bounds = { skmenu_bounds.x + skmenu_bounds.w, skmenu_bounds.y + (skmenu_bounds.h) / 2
            sksetmenu.settext( string_format( _( "Set '%s' to…" ), skill.name() ) );
            const int skcur = get_level( skill );
            sksetmenu.selected = skcur;
            for( int i = 0; i <= MAX_SKILL; i++ ) {
                sksetmenu.addentry( i, true, i + 48, "%d%s", i, skcur == i ? _( " (current)" ) : "" );
            }
            sksetmenu.query();
            skset = sksetmenu.ret;
        }

        if( skill_id >= 0 && skset >= 0 ) {
            const Skill &skill = *sorted_skills[skill_id];
            if( change_theory ) {
                you->set_knowledge_level( skill.ident(), skset );
            } else {
                you->set_skill_level( skill.ident(), skset );
            }
            skmenu.text = string_format( _( "%s set to %d" ), skill.name(), get_level( skill ) );
            skmenu.entries[skill_id + skoffset].txt = string_format( _( "@ %d: %s  " ),
                    get_level( skill ),
                    skill.name() );
            skmenu.entries[skill_id + skoffset].text_color =
                get_level( skill ) == origskills[skill_id] ?
                skmenu.text_color : c_yellow;
        } else if( skmenu.ret == 0 && sksel == -1 ) {
            const int ret = uilist( _( "Alter all skill values" ), {
                _( "Add 3" ), _( "Add 1" ),
                _( "Subtract 1" ), _( "Subtract 3" ), _( "Set to 0" ),
                _( "Set to 5" ), _( "Set to 10" ), _( "(Reset changes)" )
            } );
            if( ret >= 0 ) {
                int skmod = 0;
                int skset = -1;
                if( ret < 4 ) {
                    skmod = 3 - ret * 2;
                } else if( ret < 7 ) {
                    skset = ( ret - 4 ) * 5;
                }
                for( size_t skill_id = 0; skill_id < sorted_skills.size(); skill_id++ ) {
                    const Skill &skill = *sorted_skills[skill_id];
                    int changeto = skmod != 0 ? get_level( skill ) + skmod :
                                   skset != -1 ? skset : origskills[skill_id];

                    if( change_theory ) {
                        you->set_knowledge_level( skill.ident(), std::max( 0, changeto ) );
                    } else {
                        you->set_skill_level( skill.ident(), std::max( 0, changeto ) );
                    }

                    skmenu.entries[skill_id + skoffset].txt = string_format( _( "@ %d: %s  " ),
                            get_level( skill ), skill.name() );

                    you->get_skill_level_object( skill.ident() ).practice();
                    skmenu.entries[skill_id + skoffset].text_color =
                        get_level( skill ) == origskills[skill_id] ? skmenu.text_color : c_yellow;
                }
            }
        }
    } while( skmenu.ret != UILIST_CANCEL );
}

void debug_menu::wishproficiency( Character *you )
{
    bool know_all = true;
    const int proffset = 1;

    uilist prmenu;
    prmenu.text = _( "Select proficiency to toggle" );
    prmenu.allow_anykey = true;
    prmenu.addentry( 0, true, '1', _( "Toggle all proficiencies" ) );

    const std::vector<proficiency_id> &known_profs = you->known_proficiencies();
    std::vector<std::pair<proficiency_id, bool>> sorted_profs;

    for( const proficiency &cur : proficiency::get_all() ) {

        const auto iterator = std::find_if( known_profs.begin(), known_profs.end(),
        [&cur]( proficiency_id prof_id ) {
            return cur.prof_id() == prof_id;
        } );

        const bool player_know = iterator != known_profs.end();

        // Does the player know all proficiencies
        if( know_all ) {
            know_all = player_know;
        }

        sorted_profs.emplace_back( cur.prof_id(), player_know );
    }

    std::sort( sorted_profs.begin(), sorted_profs.end(), [](
    const std::pair<proficiency_id, bool> &profA,  const std::pair<proficiency_id, bool> &profB ) {
        return localized_compare( profA.first->name(), profB.first->name() );
    } );

    for( size_t i = 0; i < sorted_profs.size(); ++i ) {
        if( sorted_profs[i].second ) {
            prmenu.addentry( i + proffset, true, -2, _( "(known) %s" ),
                             sorted_profs[i].first->name() );
            prmenu.entries[i + proffset].text_color = c_yellow;
        } else {
            prmenu.addentry( i + proffset, true, -2, _( "%s" ),
                             sorted_profs[i].first->name() );
            prmenu.entries[i + proffset].text_color = prmenu.text_color;
        }
    }

    do {
        prmenu.query();
        const int prsel = prmenu.ret;
        if( prsel == 0 ) {
            // if the player knows everything, unlearn everything
            if( know_all ) {
                for( size_t i = 0; i < sorted_profs.size(); ++i ) {
                    std::pair<proficiency_id, bool> &cur = sorted_profs[i];
                    cur.second = false;
                    prmenu.entries[i + proffset].txt = string_format( "%s",  cur.first->name() );
                    prmenu.entries[i + proffset].text_color = prmenu.text_color;
                    you->lose_proficiency( cur.first, true );
                }
                know_all = false;
            } else {
                for( size_t i = 0; i < sorted_profs.size(); ++i ) {
                    std::pair<proficiency_id, bool> &cur = sorted_profs[i];

                    if( !cur.second ) {
                        cur.second = true;
                        prmenu.entries[i + proffset].txt = string_format( _( "(known) %s" ), cur.first->name() );
                        prmenu.entries[i + proffset].text_color = c_yellow;
                        you->add_proficiency( cur.first, true );
                    }
                }
                know_all = true;
            }
        } else if( prsel > 0 ) {
            std::pair<proficiency_id, bool> &cur = sorted_profs[prsel - proffset];
            // if the player didn't know it before now it does
            // if the player knew it before, unlearn proficiency
            bool know_prof = !cur.second;
            proficiency_id &prof = cur.first;

            cur.second = know_prof;

            if( know_prof ) {
                prmenu.entries[prmenu.selected].txt = string_format( _( "(known) %s" ), cur.first->name() );
                prmenu.entries[prmenu.selected].text_color = c_yellow;
                you->add_msg_if_player( m_good, _( "You are now proficient in %s!" ), prof->name() );
                you->add_proficiency( prof, true );
                continue;
            }

            know_all = false;
            prmenu.entries[prmenu.selected].txt = string_format( "%s", cur.first->name() );
            prmenu.entries[prmenu.selected].text_color = prmenu.text_color;
            you->add_msg_if_player( m_bad, _( "You are no longer proficient in %s." ), prof->name() );
            you->lose_proficiency( prof, true );
        }
    } while( prmenu.ret != UILIST_CANCEL );
}
