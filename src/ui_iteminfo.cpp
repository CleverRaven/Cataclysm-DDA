#include "ui_iteminfo.h"

#include "messages.h"
#include "ui_manager.h"
#include "imgui/imgui_internal.h"


void draw_item_info_imgui( cataimgui::window &window, item_info_data &data, int width )
{
    float wrap_width = window.str_width_to_pixels( width - 2 );
    nc_color base_color = c_light_gray;

    if( !data.get_item_name().empty() ) {
        window.draw_colored_text( data.get_item_name(), base_color, wrap_width );
        ImGui::Spacing();
    }
    if( !data.get_type_name().empty() &&
        data.get_item_name().find( data.get_type_name() ) == std::string::npos ) {
        window.draw_colored_text( data.get_type_name(), base_color, wrap_width );
    }

    bool same_line = false;
    for( const iteminfo &i : data.get_item_display() ) {
        if( i.sType == "DESCRIPTION" ) {
            if( i.bDrawName ) {
                if( i.sName == "--" ) {
                    ImGui::Separator();
                } else {
                    window.draw_colored_text( i.sName, base_color, wrap_width );
                }
            }
        } else {
            std::string formatted_string;
            if( i.bDrawName ) {
                formatted_string = i.sName;
            }

            std::string sFmt = i.sFmt;

            //A bit tricky, find %d and split the string
            size_t pos = sFmt.find( "<num>" );
            if( pos != std::string::npos ) {
                formatted_string += sFmt.substr( 0, pos );
            } else {
                formatted_string += sFmt;
            }

            if( i.sValue != "-999" ) {
                nc_color this_color = c_yellow;
                for( const iteminfo &k : data.get_item_compare() ) {
                    if( k.sValue != "-999" ) {
                        if( i.sName == k.sName && i.sType == k.sType ) {
                            double iVal = i.dValue;
                            double kVal = k.dValue;
                            if( i.sFmt != k.sFmt ) {
                                // Different units, compare unit adjusted vals
                                iVal = i.dUnitAdjustedVal;
                                kVal = k.dUnitAdjustedVal;
                            }
                            if( iVal > kVal - .01 &&
                                iVal < kVal + .01 ) {
                                this_color = c_light_gray;
                            } else if( iVal > kVal ) {
                                if( i.bLowerIsBetter ) {
                                    this_color = c_light_red;
                                } else {
                                    this_color = c_light_green;
                                }
                            } else if( iVal < kVal ) {
                                if( i.bLowerIsBetter ) {
                                    this_color = c_light_green;
                                } else {
                                    this_color = c_light_red;
                                }
                            }
                            break;
                        }
                    }
                }
                formatted_string += colorize( i.sValue, this_color );
            }

            if( pos != std::string::npos ) {
                formatted_string += sFmt.substr( pos + 5 );
            }

            if( !formatted_string.empty() ) {
                if( same_line ) {
                    ImGui::SameLine( 0, 0 );
                }
                window.draw_colored_text( formatted_string, base_color, wrap_width );
            }
            same_line = !i.bNewLine;
        }
    }
}

iteminfo_window::iteminfo_window( item_info_data &info, point pos, int width, int height,
                                  ImGuiWindowFlags flags ) :
    cataimgui::window( "iteminfo", flags ),
    data( info ), pos( pos ), width( width ), height( height )
{
    ctxt = input_context( "default" );
    if( info.handle_scrolling ) {
        ctxt.register_action( "PAGE_UP" );
        ctxt.register_action( "PAGE_DOWN" );
    }
    ctxt.register_action( "CONFIRM" );
    ctxt.register_action( "QUIT" );
    if( info.any_input ) {
        ctxt.register_action( "ANY_INPUT" );
    }

    ctxt.set_timeout( 10 );
}

cataimgui::bounds iteminfo_window::get_bounds()
{
    return { pos.x < 0 ? -1.f : static_cast<float>( str_width_to_pixels( pos.x ) ),
             pos.y < 0 ? -1.f : static_cast<float>( str_height_to_pixels( pos.y ) ),
             static_cast<float>( str_width_to_pixels( width ) ),
             static_cast<float>( str_height_to_pixels( height ) ) };
}

void iteminfo_window::draw_controls()
{
    draw_item_info_imgui( *this, data, width );

    if( scroll != 0 ) {
        float scroll_pos = ImGui::GetScrollY() + scroll *
                           ImGui::GetCurrentContext()->CurrentWindow->InnerRect.GetHeight();
        ImGui::SetScrollY( scroll_pos );
        scroll = 0;
    }
}

void iteminfo_window::execute()
{
    if( data.without_getch ) {
        ui_manager::redraw();
        return;
    }

    while( true ) {
        ui_manager::redraw();
        std::string action = ctxt.handle_input();

        if( data.handle_scrolling && action == "PAGE_UP" ) {
            scroll = -1;
        } else if( data.handle_scrolling && action == "PAGE_DOWN" ) {
            scroll = 1;
        } else if( action == "CONFIRM" || action == "QUIT" ||
                   ( data.any_input && action == "ANY_INPUT" && !ctxt.get_raw_input().sequence.empty() ) ) {
            break;
        }

        // mouse click on x to close leads here
        if( !get_is_open() ) {
            break;
        }
    }
}
